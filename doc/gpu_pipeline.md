# The GPU pipeline in ART

ART's Vulkan backend lives entirely under `rtengine/gpu/`. It's built around
one hard rule stated at the top of `gpu.h`: **nothing here ever throws or
aborts**. Every entry point returns `bool`/pointer, and a `false`/`null` means
"run the existing CPU code instead." The GPU path is always an optional
accelerator bolted onto a CPU pipeline that still works without it.

This document assumes no prior Vulkan knowledge and explains each concept the
first time a file uses it.

## 1. What Vulkan actually requires (the concepts these files wrap)

Vulkan is a low-level, explicit GPU API. Unlike OpenGL, almost nothing is
implicit or global — you build every object yourself and the driver does very
little bookkeeping for you. The pieces ART needs are:

- **Loader/ICD**: a shared library (`libvulkan.so`, `vulkan-1.dll`, or on
  macOS `libMoltenVK.dylib`, since Apple has no native Vulkan and MoltenVK
  translates Vulkan calls to Metal) that you must locate and open yourself.
- **Instance**: the top-level Vulkan "session" for the process.
- **Physical device**: a GPU the instance can see (there can be several; a
  laptop might report an integrated and a discrete GPU, plus software
  rasterizers like `lavapipe`).
- **Logical device**: the object you actually issue commands through,
  created from a physical device.
- **Queue**: where you submit work; queues belong to a "queue family" that
  advertises capabilities (graphics, compute, transfer, ...).
- **Buffer + device memory**: Vulkan separates the "handle that describes a
  buffer's shape" (`VkBuffer`) from the "actual memory backing it"
  (`VkDeviceMemory`) — you allocate memory yourself and explicitly bind it to
  the buffer.
- **Shader module / SPIR-V**: compute shaders are compiled ahead of time to
  SPIR-V bytecode (via `glslc`, see the shaders section below) and loaded as
  opaque blobs.
- **Descriptor set / descriptor set layout**: the mechanism for telling a
  shader "these buffers are bound to these binding-number slots." A layout is
  the schema; a set is the actual pointer-to-buffers instance of it.
- **Pipeline / pipeline layout**: a compiled, ready-to-run configuration of a
  shader plus its descriptor-set layout and push-constant layout.
- **Push constants**: a small (spec-guaranteed ≥128 bytes), fast, per-dispatch
  parameter block — cheaper than a buffer for a handful of scalars.
- **Command buffer**: you don't call GPU functions directly; you *record*
  commands (bind pipeline, bind descriptors, dispatch, insert barriers) into a
  command buffer, then submit the whole buffer to a queue.
- **Fence**: a CPU-visible signal you wait on to know a submission finished.
- **Pipeline barrier**: an explicit synchronization point telling the GPU
  "don't start this access until that access is complete" — Vulkan does *no*
  automatic hazard tracking between dispatches, unlike CPU cache coherency.
- **Specialization constants**: compile-time-ish constants baked into a
  pipeline at creation (not at shader-source-compile time), used here so one
  SPIR-V module can be instantiated with different workgroup sizes per
  device.

With that vocabulary, here's how ART's files map onto it.

## 2. Layered structure

```
gpu.h / gpu.cc          — public facade; no Vulkan types leak past here
vk_api.h / .cc          — dlopen the loader, resolve function pointers
vk_context.h / .cc      — instance, physical/logical device, memory, Buffer, BufferPool
vk_pipeline.h / .cc     — SPIR-V lookup, descriptor-set-layout/pipeline cache
vk_pass.h / .cc         — command buffer recording, barriers, dispatch, timing
residency.h / .cc       — per-Imagefloat CPU/GPU state machine
ops.h / .cc             — small shared multi-dispatch helpers (log transform, colorspace, ...)
shaders/*.comp          — the actual compute kernels, compiled to SPIR-V and embedded
```

Each layer only depends on the ones above it in this list; each op
implementation (e.g. `ipsmoothing.cc`) sits on top of all of them.

### 2.1 `vk_api.h`/`vk_api.cc` — loading Vulkan without linking it

ART never links `libvulkan`. `vk_api.cc` `dlopen`s a loader by trying a list
of candidate paths (MoltenVK first on macOS — see `vk_api.cc:75-91`), so a
machine with no Vulkan installed just fails that open and ART carries on with
the CPU path. `ART_VULKAN_LIBRARY` can override the search entirely.

Function pointers are declared as ordinary-looking globals (`vkCreateInstance`,
`vkQueueSubmit`, etc. — see the `ART_VK_*_FUNCS` X-macros in
`vk_api.h:43-118`) so every other file in the backend can call Vulkan
functions as if they were linked normally. Resolution happens in three tiers
matching Vulkan's own dispatch model:

1. **Global** functions (`vk_api.cc:135-138`) — resolved via
   `vkGetInstanceProcAddr(NULL, name)`, before any instance exists (e.g.
   `vkCreateInstance` itself).
2. **Instance** functions (`loadInstanceFuncs`, `vk_api.cc:147`) — resolved
   via `vkGetInstanceProcAddr(instance, name)` once an instance exists.
3. **Device** functions (`loadDeviceFuncs`, `vk_api.cc:158`) — re-resolved via
   `vkGetDeviceProcAddr(device, name)` once a logical device exists, which
   skips a dispatch trampoline the loader would otherwise insert on every
   call.

### 2.2 `vk_context.h`/`vk_context.cc` — the device and its memory

`Context` is the singleton that owns the Vulkan instance, the chosen physical
device, the logical device, and the single compute queue. It's created lazily
on first use (`Context::get()`, `vk_context.h:212`), guarded by a mutex, and
remembers whether the previous attempt failed so it never retries pointlessly.

**Device selection** (`pickPhysicalDevice`, `vk_context.cc:488`) walks every
physical device Vulkan reports and requires Vulkan ≥1.1 plus a queue family
with `VK_QUEUE_COMPUTE_BIT`. It rejects software rasterizers
(`lavapipe`/`llvmpipe`/`SwiftShader`, detected in `looksLikeSoftware`,
`vk_context.cc:71`) unless the user explicitly opts in with
`ART_VULKAN_ALLOW_SOFTWARE=1` — silently picking a software Vulkan device
would be slower than ART's existing OpenMP CPU path and would look like a
driver bug rather than a policy choice. Preference resolution (`"auto"`, a
numeric index, or a name substring) happens here too.

**`createInstance`** (`vk_context.cc:115`) has a documented macOS-specific
two-attempt dance: MoltenVK wants `VK_KHR_portability_enumeration` in one
configuration but a real loader wants it absent in another, so ART tries
with the extension, then without, based on measured behavior on MoltenVK
1.4.2.

**`Caps`** (`vk_context.h:37`) is a plain struct populated once in
`queryCaps()` (`vk_context.cc:568`) from `vkGetPhysicalDeviceProperties`/
`Properties2`/`MemoryProperties`. It's what every kernel-writing/dispatching
call site is supposed to check against instead of assuming a number — max
workgroup size, max push-constant bytes, whether the device has one unified
memory heap or a separate VRAM pool, etc. The comment on `unified_memory`
explains why this matters: on a unified device (integrated GPUs generally,
and Apple Silicon specifically, since MoltenVK is the loader ART talks to
there) the "device-local" heap *is* system RAM, so ART has to be more
conservative with it than a discrete GPU would need to be, because it's
competing with the CPU side of the pipeline for the same memory rather than
using dedicated VRAM.

**`Buffer`** (`vk_context.h:84`) wraps a `VkBuffer` + its `VkDeviceMemory` as
one move-only object. What memory it gets is controlled by **`HostMemoryMode`**
(`vk_context.h`), which replaced an earlier plain `bool want_host_visible`
with three explicit modes: `PREFER_DEVICE_LOCAL` (try device-local *and*
host-visible *and* host-coherent together first, falling back to device-local
alone), `DEVICE_LOCAL_ONLY`, and `STAGING_ONLY` (requires `HOST_VISIBLE|
HOST_COHERENT`, with no tier requiring `DEVICE_LOCAL` — every conformant
device has *some* host-visible memory type, even a discrete GPU with no
resizable BAR). `Context::createBuffer(bytes, bool)` still exists as a
compatibility overload mapping `true`/`false` to
`PREFER_DEVICE_LOCAL`/`DEVICE_LOCAL_ONLY`.

On a unified-memory device the device-local-and-host-visible-and-host-coherent
combination always exists, so `PREFER_DEVICE_LOCAL` buffers are mapped once
and left mapped forever (`vkMapMemory`, `vk_context.cc:931`), and
`Buffer::mapped()` is never null. That's why plain `memcpy` shows up so often
in op code: no staging buffer or transfer command is involved at all on that
class of device. A genuine discrete GPU usually has no such combined type, so
a `PREFER_DEVICE_LOCAL` buffer's `mapped()` comes back null there, and callers
need the staging path below instead of dereferencing it directly.

**`Pass::copyBuffer`** (`vk_pass.h`/`.cc`) records a `vkCmdCopyBuffer` through
the same `barrierFor()` machinery `fillBuffer` already used for its
transfer-stage command — no changes were needed to barrier derivation itself
to support it.

**`uploadToBuffer`/`downloadFromBuffer`/`copyBufferToBuffer`**
(`vk_context.h`/`.cc`) are the synchronous helpers built on `copyBuffer`: when
the buffer involved is mapped they're a plain memcpy (the unified-memory fast
path, unchanged); when it isn't, they open an internal `Pass` and copy
through a host-visible staging buffer instead — `copyBufferToBuffer` can also
go device-to-device with no host trip at all, which is useful even on unified
memory. `Context::stagingPoolForThisThread()` hands out a thread-local
`BufferPool` constructed with `HostMemoryMode::STAGING_ONLY`, mirroring
`commandPoolForThisThread()`'s pattern, for callers (op-file scratch,
`plane_io.h`, see §2.6) that don't want to manage their own staging buffer.
Each helper wraps its checkout in a `PoolScope` so the buffer goes back on
every exit path — without that the pool only ever grows, since `get()` reuses
an entry only once it has been released, and every staged transfer would
allocate fresh device memory and hold it for the life of the thread. That is
the opposite of what `BufferPool` is for, and it stayed invisible here for
exactly the reason the whole staging path does: the mapped fast path means
`get()` is never reached on this machine. `ART_GPU_FORCE_DISCRETE_STAGING=1`
plus the allocation counters in the `-V` report is how to see it (§4).

Which tier `PREFER_DEVICE_LOCAL` may take is now a decision rather than an
accident. `Context::hostVisibleDeviceLocalWanted()` gates the
`DEVICE_LOCAL|HOST_VISIBLE` tiers on `Caps::unified_memory` (overridable with
`ART_GPU_HOST_VISIBLE_DEVICE_LOCAL=0/1`). On a unified device, mapping device
memory is simply how the hardware works and makes every transfer a memcpy with
no submission.

`Caps::unified_memory` is derived from the Vulkan device type, **not** from the
memory types, and this distinction was learned the hard way. The separate
`Caps::host_visible_device_local` records whether any `DEVICE_LOCAL|
HOST_VISIBLE` type exists — which is true on essentially every discrete GPU,
because NVIDIA always exposes the BAR window as one (256 MB without resizable
BAR, all of VRAM with it). An RTX 4500 Ada therefore reports that flag set, and
an earlier version of this gate keyed off it and so did nothing at all on the
hardware it was written for. The banner prints `unified memory` or
`host-visible VRAM` to keep the two visibly distinct.

On a discrete GPU that memory type is the PCIe BAR window, and the expectation
was that mapping it would be a trap — the pointer is write-combined and
uncached, so host *reads* in `ImageResidency::download()` and `downloadPlane()`
cross PCIe uncached. **Measured on an RTX 4500 Ada with resizable BAR, that is
wrong**: forcing those transfers through the staging path took one export from
16.4 s to 54.7 s, with `denoise:out` alone going 1.5 s → 33.8 s.

The reason is the staging path, not the BAR. `plane_io.h` packs each plane into
a fresh zero-initialised `std::vector`, memcpys that into a staging buffer, and
only then issues the device copy — three passes over 169 MB per plane per
direction, plus an allocation, against the single memcpy the mapped path does.
So the default is to map wherever the device offers it, and
`ART_GPU_HOST_VISIBLE_DEVICE_LOCAL=0` is how to re-measure staging once
`plane_io.h` stops copying three times. Without resizable BAR the window is
only ~256 MB, so a large image would fail the allocation and stage anyway;
that case has not been measured.

**`BufferPool`** (`vk_context.h:148`) exists purely for performance:
allocating a fresh `VkDeviceMemory` per call was measured as ~28% of wall
time in the wavelet denoise port (first-touch page faults dominate). A pool
keeps buffers around and just marks them reusable (`recycle()`/`PoolScope`)
instead of freeing them, with two invariants documented in the header: never
recycle before `submitAndWait()` finishes (or you'd hand live memory to a new
use while `Pass` still thinks the old contents matter for barrier purposes),
and never let addresses move (hence `vector<unique_ptr<Entry>>` rather than
`vector<Buffer>` — `Pass` tracks buffers *by pointer*). A pool is fixed to one
`HostMemoryMode` for its lifetime; `get()` (`vk_context.cc`) only requires the
buffer it hands back to be mapped when the pool's mode is `STAGING_ONLY`
(true by construction there, since every tier that mode accepts is
host-visible). A `PREFER_DEVICE_LOCAL` pool — the kind every op file's
compute scratch uses — may legitimately hand back an unmapped buffer on a
discrete GPU, so every consumer across `wavelet.cc`, `ipdenoise.cc`,
`nlmeans.cc` and `ipsmoothing.cc` has had to be read function-by-function and
sorted into two buckets: pure GPU-resident scratch (only ever bound to a
dispatch, e.g. wavelet band buffers, MAD histograms) needs nothing beyond
that relaxed check, while a buffer the CPU touches directly — uploading
initial data into it, or reading a reduction/histogram back out of it —
had to be migrated to `uploadToBuffer`/`downloadFromBuffer` against a
`STAGING_ONLY` pool, exactly like the row-packing helpers in §2.6. Getting
this wrong is not cosmetic: an early attempt at the relaxed check, before
every consumer had been audited, turned one unaudited caller's `.mapped()`
dereference into a null-pointer crash under `ART_GPU_FORCE_DISCRETE_STAGING=1`
(see §4) rather than the safe "`get()` returns null, decline to CPU" the
mapped-only check used to guarantee everywhere.

### 2.3 `vk_pipeline.h`/`vk_pipeline.cc` — turning a kernel name into something dispatchable

Each `.comp` shader source is compiled offline by `glslc` to SPIR-V at build
time and embedded directly into the binary as byte arrays
(`gpu_shaders_generated.cc`, generated from `rtengine/gpu/shaders/*.comp` via
a `file(GLOB)` at configure time — see §4 for why that means adding/removing
a `.comp` needs a re-`cmake`, while editing one's contents doesn't).
`findShader()` (`vk_pipeline.cc:67`) looks
the name up there, but first checks `ART_SHADER_PATH/<name>.spv` on disk — a
hand-compiled override for iterating on a shader without a full rebuild.

`getComputePipeline()` (`vk_pipeline.cc:103`) is a cache keyed on
`{name, n_bindings, push_size, lx, ly}` (the `Key` struct at
`vk_pipeline.cc:45`). Building a pipeline from scratch means:

1. Create a `VkShaderModule` from the SPIR-V words.
2. Create a `VkDescriptorSetLayout` describing `n_bindings` storage-buffer
   slots (bindings `0..n-1`, all `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER`,
   `vk_pipeline.cc:154-161`).
3. Create a `VkPipelineLayout` from that set layout plus a push-constant
   range.
4. Bake the device's preferred workgroup size (`lx`,`ly`) into the pipeline
   as specialization constants 0 and 1 — this is what
   `layout(local_size_x_id = 0, local_size_y_id = 1)` in a shader means: "my
   workgroup dimensions are supplied externally," which lets one SPIR-V
   module run efficiently on a tiny 8×8 Intel iGPU tile size and a 16×16
   discrete-GPU tile size without recompiling the shader source.
5. Call `vkCreateComputePipelines`, using the device's `VkPipelineCache`
   (persisted to/from disk in `Context`, see `loadPipelineCacheFromDisk`,
   `vk_context.cc:950`, and `flushPipelineCacheToDisk`, `vk_context.cc:1013`)
   so repeated runs skip most of the driver's compile cost. The cache file is
   keyed by the device's `pipelineCacheUUID` in its filename so a driver
   update or GPU swap can't feed stale bytes to `vkCreatePipelineCache`.

Failures are cached too (as an invalid `ComputePipeline` in the map) so a
missing kernel costs one map lookup per call, not a repeated failed build
attempt.

The header comment is explicit about a correctness trap here: the `(lx, ly)`
passed to `getComputePipeline` and the dispatch's group-count math (in
`vk_pass.cc`) **must be derived from the same numbers** — a mismatch doesn't
crash, it silently computes over the wrong footprint or (for binding numbers
specifically) causes a device loss several dispatches later.

### 2.4 `vk_pass.h`/`vk_pass.cc` — recording, barriers, submission, timing

`Pass` is the workhorse: "one command buffer, many dispatches, one
submission." Its constructor (`Pass::begin()`, `vk_pass.cc:56`) allocates a
command buffer from a **per-thread** command pool
(`Context::commandPoolForThisThread()`, `vk_context.cc:816` — command pools
aren't thread-safe, so each `ThreadPool` worker gets its own, lazily, via
`thread_local`), a small descriptor pool sized for at most 64 dispatches × 8
bindings each, a fence, and (if the device supports it) a timestamp query
pool for profiling.

**Dispatch** (`dispatch2D`, `vk_pass.cc:278`; `dispatch1D`, `vk_pass.cc:247`)
rounds the requested `W×H` (or flat `n`) work extent up to a whole number of
workgroups of the device's preferred/derived local size, fetches the
pipeline, allocates and writes a descriptor set binding each `Pass::Binding`'s
buffer to its positional slot, records a barrier (see below), then
`vkCmdBindPipeline` → `vkCmdBindDescriptorSets` → optional
`vkCmdPushConstants` → `vkCmdDispatch`. `dispatchRaw` is the shared
low-level implementation both call into.

**Barriers are derived, not hand-written** — this is the most important
correctness mechanism in the file. `barrierFor()` (`vk_pass.cc:164`) keeps a
list of `{buffer pointer, was-it-written}` for every buffer touched so far
*in this Pass*. Before each new dispatch, for every binding it looks up
whether that buffer was touched before, and if the old or new access is a
write (i.e. any of W→R, R→W, W→W), it emits one `VkBufferMemoryBarrier` —
R→R needs nothing. All the barriers for one dispatch are coalesced into a
single `vkCmdPipelineBarrier`. The comment explains a subtlety: `fillBuffer()`
(a `vkCmdFillBuffer`, used to zero histogram buffers without a host `memset`
breaking a device-only phase) executes in the Vulkan *transfer* stage, not
compute, so the stage masks used here always include both
`COMPUTE_SHADER` and `TRANSFER` — a compute→compute-only barrier would not
order a shader read against a preceding fill. The reasoning in the header for
centralizing this: "a missing barrier is the archetypal bug that only
reproduces on one vendor," so it's deliberately not left to each op author to
get right by hand.

**Batching is the caller's job, and it matters.** One `Pass` per kernel is the
single biggest avoidable cost in this backend: each submission drains the queue
and blocks the calling thread, and the wavelet primitive was once measured at
231 round trips summing to 69.5 ms of wall for 10.4 ms of actual compute — 85%
overhead. `PassSeq` (`vk_pass.h`) exists for this: `reserve(n)` hands back a
`Pass` with room for `n` more dispatches, opening a new one only when the
64-dispatch budget runs out, so a caller records a whole chain and submits
once. The helpers in `ops.cc`, `guidedfilter.cc` and `ipsmoothing.cc`
therefore come in two forms: a `Pass &` form that only records, and a
`Context &` form that opens its own `Pass` and submits. **Prefer the recording
form**; the submitting one is for a single isolated kernel. A
boundary is only genuinely necessary where the CPU reads a result back
mid-algorithm — `waveletShrink` needs `maxAbs` and the MAD median on the host
to compute the next stage's parameters, which is a real GPU→CPU→GPU dependency
rather than batching laziness.

**`Pass::updateBuffer`** records host data (up to 64 KiB) straight into the
command buffer via `vkCmdUpdateBuffer`. `uploadToBuffer` takes an optional
`Pass *` and uses it automatically when the payload fits, which is what keeps a
LUT or parameter-block upload from becoming a queue round trip of its own on a
device where the destination is not host-visible. `nlmeans.cc:592` is the
live case: `detailMask`'s Gaussian kernel is a handful of floats, so it rides
inside the command buffer instead of becoming a staged submission of its own.

**Submission** (`submitAndWait()`, `vk_pass.cc:427`) ends the command buffer,
resets the fence, calls `Context::submit()` (the *only* place `vkQueueSubmit`
is called anywhere in the codebase — serialized behind a mutex because
Vulkan queues aren't thread-safe, but never held across the wait), then
`vkWaitForFences` with a 10-second timeout. A `VK_ERROR_DEVICE_LOST` or a
timeout both call `Context::markDeviceLost()`, which latches — once a device
is lost, `available()` in `gpu.cc` returns `false` for the rest of the
process and every future GPU call declines immediately rather than retrying
a dead device. On a timeout specifically, the comment explains why `~Pass()`
then *leaks* its Vulkan handles rather than destroying them: the command
buffer might still be referenced by a queue that never signaled, and
destroying live Vulkan objects out from under an in-flight submission is
undefined behavior — a one-time leak is the safe choice once the GPU is being
abandoned for the rest of the session anyway.

**Timing**: if timestamps are usable (`Caps::timestampsUsable()`), each
dispatch brackets itself with `vkCmdWriteTimestamp` calls; after the wait,
`vkGetQueryPoolResults` converts raw ticks to milliseconds via
`timestamp_period_ns` (this is what `-V -V`'s `log_lin took 1.46 ms (GPU)`
lines come from, per `reportTimings()`). The wall-clock submit→wait span is
always recorded too and always printed, specifically because timestamp
granularity is coarse on some drivers — "a plausible-looking but wrong GPU
time is worse than none."

Queries 0 and 1 bracket the whole command buffer, which is what makes
`Pass::deviceMs()` meaningful: `wallMs() - deviceMs()` is the per-submission
overhead. `gpu::stats()` accumulates both, plus submission and allocation
counts, across a run, and `PipelineTimeReport` prints them under the
`pipeline (...)` line. Keeping the two apart is not cosmetic — the older report
called the submit→wait total "GPU time, i.e. actual device compute", which is
true only on unified memory. On a discrete GPU that number also contains submit
latency, fence-signal latency and every staged PCIe transfer, all of it
attributed to the kernels.

### 2.5 `residency.h`/`residency.cc` — keeping CPU and GPU in sync per-image

This is the layer that makes partial GPU porting viable. Every `Imagefloat`
owns one `ImageResidency`, which owns exactly one device `Buffer` for the
image's lifetime and tracks a three-state enum (`residency.h:57`):

- `CPU_ONLY` — CPU planes are truth, device buffer stale.
- `GPU_ONLY` — device buffer is truth, CPU planes stale.
- `BOTH` — identical; free to read from either side.

The motivating measurement (in the header comment) is that allocating a
fresh device buffer per operator call dominated cost, so instead: `forRead()`
uploads only if currently `CPU_ONLY`; `forWrite()` calls `forRead()` then
marks `GPU_ONLY`; `forDiscardWrite()` skips the upload entirely and marks
`GPU_ONLY` (used when a kernel will overwrite every pixel anyway, so
uploading first would be wasted work). Within one GPU path the pixels
therefore cross *once*, at the first `forWrite`, and back *once*, whenever
something finally needs CPU pixels via `syncToCpu()` (which is what
`Imagefloat::syncCpu*` calls at every plane-access boundary elsewhere in the
codebase).

No tool currently stays resident across an `apply()` boundary — see §3 — so
in practice each GPU path pays one upload and one download. The state machine
still earns its keep *inside* a path: `ipdenoise.cc` chains a long run of
kernels between the two transfers, and `DenoiseSession::output` leaving the
image GPU-resident is what lets `finalSmoothing` pick it straight back up
instead of downloading and re-uploading it (see §2.6).

Two defensive details worth calling out:

- **Geometry can change under it.** `ensureBuffer()` (`residency.cc:110`)
  re-checks the owner's width/height every time, because
  `PlanarRGBData::allocate` can resize an `Imagefloat` in place; a
  stale-sized buffer is discarded and residency drops back to `CPU_ONLY`.
  `staleBuffer()` guards the paths that don't go through `ensureBuffer()`
  first (`syncToCpu`, `copyTo`).
- **`poisonCpuPlanes()`** (`residency.cc:195`) — whenever the GPU becomes
  authoritative, the CPU planes get overwritten with a recognizable NaN bit
  pattern (`0x7FC0DEAD`), unconditionally in debug builds, opt-in via
  `ART_GPU_DEBUG=poison` in release. This is deliberately loud: a missed
  `syncCpu()` call anywhere in the huge CPU codebase is otherwise an
  invisible "subtly stale pixels" bug, and the comment notes this is exactly
  the class of bug that caused a real GUI-corruption regression (`ART_GPU=1`
  turning the editor black) that went unnoticed for a while.

`ImageResidency` is also the one consumer of the discrete-GPU staging path
(§2.2) that keeps its own dedicated staging buffer (`staging_buf_`, sized to
`deviceBytes()`) rather than borrowing the shared `stagingPoolForThisThread()`
pool, since it only ever needs one buffer at a time. `upload()`/`download()`
return `bool` and branch on `buf_->hostVisible()`: a plain memcpy when the
device buffer is mapped, or a staged transfer through `staging_buf_`
otherwise; `forRead()`/`syncToCpu()` only advance `loc_` on success, so a
failed staging transfer declines to the CPU exactly like any other
allocation failure rather than silently handing back stale or garbage device
contents. `copyTo()` keeps its original lock-free mapped-to-mapped memcpy
when both sides are host-visible (it runs at every pipeline stage boundary
via `Imagefloat::copyData`, so this fast path is worth keeping rather than
always going through a queue submission) and falls back to a direct
on-device `Pass::copyBuffer` otherwise.

`ResidencyGuard` (`residency.h:124`) is a small RAII helper: if a GPU op
takes the buffer for writing but then fails partway through (e.g.
`submitAndWait()` fails), its destructor calls `invalidateGPU()` unless the
op explicitly called `success()`, ensuring a failed GPU attempt doesn't leave
the image claiming GPU-authoritative garbage.

### 2.6 `ops.h`/`ops.cc` and the shaders themselves

`ops.cc` holds small, *shared* multi-dispatch building blocks used by more
than one caller — e.g. `logTransform` (wraps the `log_lin.comp` kernel),
`logGuidedFilterSelf`/`WithGuide` (log-space guided filter, used by
`ipsmoothing.cc`), and colorspace conversions (`rgb2yuv`/`yuv2rgb`/
`rgbLuminance`/`yuvRecombine`). Each function follows
the same shape: build a `Pass`, push one or more `dispatch2D` calls with a
small `PC` (push-constant) struct matching the shader's
`layout(push_constant) uniform PC {...}`, `submitAndWait()`,
`reportTimings()`.

Individual per-tool GPU implementations (like `gpu::ops::nlmeans_smoothing`
in `ipsmoothing.cc:1699`) live next to their CPU counterpart in the same
`.cc` file, gated by `#ifdef ART_USE_VULKAN`, rather than under
`rtengine/gpu/`.

The `.comp` files themselves are ordinary GLSL compute shaders.
`log_lin.comp` is representative of the whole style:
`layout(local_size_x_id = 0, local_size_y_id = 1) in` declares the workgroup
size as externally-specialized (see §2.3), `gl_GlobalInvocationID.xy` gives
each invocation its pixel coordinate, an explicit bounds check
(`if (g.x >= pc.w ...) return;`) handles the rounded-up-to-workgroup-size
overshoot, and its two `std430 ... buffer { float v[]; }` declarations at
`binding = 0` and `binding = 1` are flat storage buffers treated as plain
arrays — there's no image/texture object anywhere in this backend, just raw
float arrays, matching how `Imagefloat`'s planes are laid out in memory.

Prefer `ImageResidency` over these helpers when the pixels are a whole
`Imagefloat` rather than a standalone plane. Uploading R/G/B with
`uploadPlane` and reading them back with `downloadPlane` looks equivalent and
is not, for three reasons: `uploadPlane` allocates a fresh device buffer per
plane per call (169 MB each on a full-size image, and on a discrete GPU out of
the host-visible BAR window, crowding out later allocations); the copies are
row-by-row rather than one contiguous memcpy; and the download is
unconditional, so the result crosses PCIe even when the next kernel wanted it
on the device. Residency gives one buffer for the image's lifetime and one
contiguous transfer; `ops::transferPlane` bridges the row-padded residency
layout and the tightly-packed planes the kernels want, on-device, as
`ipsmoothing.cc`'s `imageToPlanes` does.

`DenoiseSession::output` is the measured case, and was worse than the shape
above: its planes come from a pool and are usually *not* host-visible on a
discrete GPU, so each readback took the staged path — a device→host copy, a
fresh zero-initialised `std::vector` of a whole plane, then two more memcpys.
1479 ms around a 7.2 ms kernel, and the residency trace showed the readback
was pointless as well as slow: the next step (`finalSmoothing`) called
`forWrite()` and uploaded all 507 MB straight back. It now packs into the
destination's residency buffer with `forDiscardWrite` (every pixel is
overwritten, so the upload `forWrite` would do first is pure cost) and leaves
the image resident.

Verify a change like this with `ART_GPU_DEBUG=poison`: if leaving an image
GPU-resident exposes a path that reads CPU planes without syncing, the poison
pattern reaches the output instead of the bug staying invisible.

**`plane_io.h`**'s `uploadPlane`/`uploadPlaneInto`/`downloadPlane` are the
shared helpers most op files use to move a row-pointer-indirected CPU plane
(`float **`, the layout `Imagefloat`'s L/a/b and R/G/B planes use) to and from
a device buffer: they pack/unpack into a contiguous `std::vector<float>` and
then go through `uploadToBuffer`/`downloadFromBuffer` (§2.2), so the same
function works whether the destination buffer is mapped or needs staging.
Every op file's own scratch buffers follow one of two shapes built on top of
that: pure GPU-resident scratch (e.g. `guidedfilter.cc`'s six intermediates,
`ipsmoothing.cc`'s per-plane working buffers) is only ever bound to a
dispatch and needs no upload/download at all; CPU-touched scratch (LUTs,
parameter blocks, masks, histogram/reduction readback) goes through
`plane_io.h` or a direct `uploadToBuffer`/`downloadFromBuffer` call. A few
buffers get uploaded/downloaded often enough within one session that the
pattern is wrapped once in a small helper local to that op file instead of
repeated at each call site — `ipdenoise.cc`'s `Impl::up()`/`down()` and
`DenoiseSession::syncNoisevarTo{Device,Host}`, and the row-packing
`dnUploadRows`/`dnDownloadRows` pair used by its standalone (non-session)
entry points, are examples of the same shape. One histogram that's zeroed by
the CPU before the GPU atomically increments it (`ipsmoothing.cc`'s
`histAbsFusedDispatch`) instead falls back to `Pass::fillBuffer` (§2.4) when
its buffer isn't mapped — exactly the case that primitive's own doc comment
names as its motivating use.

## 3. End-to-end walkthrough: `nlmeans_smoothing` (guided smoothing)

`ipsmoothing.cc:1699-1811` ties every layer together, and shows the shape
every GPU path in the tree now follows: a tool whose *interior* runs on the
device, entered and left through the CPU planes.

```cpp
bool nlmeans_smoothing(Imagefloat *rgb, ..., Context *ctx, BufferPool *pool)
{
    if (!ctx || !pool || !opEnabled("smoothing") || !available()) {
        return false;                    // decline; the caller runs the CPU code
    }
    const size_t bytes = size_t(W) * H * sizeof(float);

    PoolScope scope(*pool);              // §2.2: scratch goes back on every exit path
    Buffer *wRp = pool->get(bytes), *wGp = pool->get(bytes), *wBp = pool->get(bytes);
    if (!wRp || !wGp || !wBp) {
        logOnce("GPU: guidedSmoothing allocation failed; using the CPU");
        return false;
    }

    PassSeq seq(*ctx, "nlmeans_smoothing");   // §2.4: one command buffer per chain
    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        // imageToPlanes = residency().forWrite() (§2.5: upload once, mark GPU_ONLY)
        // + three ops::transferPlane dispatches (§2.6) unpacking the row-padded
        // residency buffer into tightly-packed W x H planes
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, true)) return false;
    }
    ...                                  // luminance, NL-means iterations, recombine
    {
        Pass *ps = seq.reserve(IMAGETOPLANES_DISPATCHES);
        if (!ps || !imageToPlanes(*ps, rgb, wRp, wGp, wBp, false)) return false;
    }
    if (!seq.flush()) return false;      // §2.4: submit, wait, latch device-lost on failure

    rgb->syncCpu();                      // §2.5: hand the pixels back
    return true;
}
```

The caller (`ipsmoothing.cc:519`) is a plain
`if (gpu::ops::nlmeans_smoothing(...)) return;` in front of the pre-existing
CPU implementation — the "try-then-fall-through" shape every GPU path follows.
Every `return false` above, at whatever depth, just means the CPU code runs
instead: `PoolScope` hands the scratch back, and the image's own pixels aren't
touched until the closing `imageToPlanes(..., false)`.

The single-kernel end of the same pattern is
`nlmeansSimpleOnDevice` (`nlmeans.cc:627`): build a `Pass`, push one `PC`
struct matching the shader's `layout(push_constant) uniform PC {...}`, push a
`Pass::Binding` per buffer **in the shader's binding order** (§2.3, §4),
`dispatch2D`, `submitAndWait()`, `reportTimings()`.

`opEnabled("smoothing")` is the `ART_GPU_DISABLE_OPS` hook from `gpu.cc`.
`hasFullGPUStage()` (also in `gpu.cc`) is a separate, stricter check, used by
the `apply()` funnel in `improcfun.cc:628` to decide whether a stage tool needs
a forced `syncCpuForWrite()` before it runs. **Its list is deliberately
empty.** A tool qualifies only if *every* path from its `apply()` entry point
to a kernel is free of direct CPU-plane access, and none is: the tools that
once were (exposure, saturation/vibrance, the tone equalizer) had their GPU
paths reverted after measurement showed they didn't pay for the transfers, and
their kernels are gone with them. So `apply()` syncs before every tool today,
and each GPU path re-uploads through `ImageResidency` on entry. That upload is
cheap — one buffer for the image's lifetime, one contiguous transfer — but it
isn't free, which is the whole reason the list exists; re-populating it is the
lever for a tool that turns out to be worth running end to end on the device.

## 4. Pitfalls, environment variables, and testing the discrete-GPU path

A few correctness pitfalls fall directly out of the mechanisms above, and are
easy to hit while editing a shader or its call site:

- **Bindings must be contiguous `0..N-1`** (`vk_pipeline.cc:154-161`,
  `vk_pass.cc:347`) — `dstBinding` is assigned positionally from the
  `bindings` vector's index, so the *order you push `Pass::Binding`s in* is
  the actual binding contract, independent of what number you write in the
  shader's `layout(binding = N)`. This bites when a kernel is derived from a
  wider one by dropping inputs and the survivors keep their old numbers; the
  symptom is not a validation error and not a wrong pixel, it's a device loss
  several dispatches later (see below), which reads like a driver problem and
  sends you looking at the wrong kernel.
- **The shader list is a `file(GLOB)`** (`rtengine/CMakeLists.txt`) — matches
  `findShader()`'s embedded-blob path (`gpu_shaders_generated.cc`, §2.3)
  being generated at configure time from whatever `.comp` files exist then.
  Adding or deleting a shader needs a re-`cmake`; editing one's contents
  doesn't.
- **"device lost" in `vkWaitForFences`** is exactly the failure path in
  `Pass::submitAndWait()` (§2.4) — both a real device hang and the
  binding-mismatch case above surface this way, so a device loss right after
  a new or edited shader is worth diffing the shader's `binding =` numbers
  against the caller's `Pass::Binding` push order before assuming a driver
  bug.
- **The GPU may already be on regardless of `ART_GPU`.** A `GPUDevice=auto`
  setting in ART's own persisted options (wired to
  `rtengine::Settings::gpu_device`) ties directly into
  `Context::configure`/`pickPhysicalDevice`'s `"auto"` preference logic in
  `vk_context.cc` — on a machine where that's the saved preference, *every*
  run already uses the GPU, and a run with `ART_GPU` simply unset is not a
  CPU reference. `ART_GPU=0` is the only reliable way to force the real CPU
  baseline; `ART_GPU=1`/`force` upgrades an `off`/empty preference to `auto`,
  and a device name selects a specific one.

A handful of other environment variables gate behavior described above:
`ART_VULKAN_DEVICE=<index|name>` picks a device; `ART_VULKAN_ALLOW_SOFTWARE=1`
allows a software Vulkan device past `pickPhysicalDevice`'s rejection (§2.2);
`ART_GPU_DISABLE_OPS=a,b` declines the named ops via `opEnabled()` (§3, note
that some GPU entry points don't consult it); `ART_GPU_DEBUG=poison` opt-in
enables `poisonCpuPlanes()` in release builds (§2.5, on unconditionally in
debug); `ART_GPU_TRACE=1` traces `ImageResidency` state transitions;
`ART_SHADER_PATH=<dir>` loads `<name>.spv` from disk instead of the embedded
blob (§2.3); `ART_VULKAN_LIBRARY=<path>` overrides the loader dylib (§2.1);
and `ART_VULKAN_VALIDATION=1` requests the `VK_LAYER_KHRONOS_validation`
layer, which does nothing on a machine where that layer isn't installed —
MoltenVK in particular bypasses the loader entirely, so validation layers
are never available through it regardless of this flag.

**`ART_GPU_FORCE_DISCRETE_STAGING=1`** is the one specific to this section:
it makes every `createBuffer(bytes, PREFER_DEVICE_LOCAL)` call skip straight
to the `DEVICE_LOCAL`-only tier, so `mapped()` comes back null and the
staging path in §2.2/§2.5/§2.6 runs even on a unified-memory device that
would otherwise never exercise it. It is the way to test that path anywhere
a discrete GPU isn't available: run the same `-p <profile>` A/B diff used for
any GPU op (export with and without the environment variable, diff the
TIFFs — see §3 for the general shape of a GPU path, and compare against a
plain `ART_GPU=0` run for the true CPU baseline) and confirm two things — the
forced run doesn't crash or log a device loss, and its output matches the
unforced run within the same few-LSB, run-to-run noise floor any two GPU runs
of the same image show (pixel results are not
bit-reproducible run to run on this backend; a one-count difference between
two runs of the *same* arm is noise, not a regression). `BufferPool::get()`'s
per-mode `mapped()` requirement (§2.2) is exactly the kind of thing this flag
is for: relaxing it without first auditing every `PREFER_DEVICE_LOCAL`
consumer turns an unaudited caller's direct `.mapped()` dereference into a
null-pointer crash under forcing instead of a safe decline-to-CPU, which is
the kind of regression this flag is meant to catch before it ships.
Real discrete-GPU hardware still needs to verify this independently, since
the flag only reproduces the *memory-layout* difference (unmapped
`PREFER_DEVICE_LOCAL` buffers) — it says nothing about a real discrete GPU's
PCIe transfer latency, driver quirks, or `VK_LAYER_KHRONOS_validation`
coverage, none of which a unified-memory device can exercise on its own.

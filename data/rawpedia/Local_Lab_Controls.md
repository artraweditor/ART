---
title: Local Lab Controls
contributors:
  - Sguyader
  - Jdc
  - Keno40
  - DrSlony
---

## What kind of local control?

Several types of local control exist in mainstream applications :

1.  “lasso”, associated with layers and fusion masks (e.g. Photoshop©).
    This kind of control is widespread (Photoshop, Gimp, Darktable…) and
    users tend to favor those, at the expense of the type described
    in 3. below, which is less well known;
2.  dedicated tools for the removal of image defects such as “red eyes”,
    or “spots” associated with sensor imperfections or dirt;
3.  “U-points” controls, used until recently in Nikon Capture NX2©, or
    as an add-on to other software such as Nik Software©. For those
    (rare now) who tried such programs, there’s no way turning back to
    layers and fusion masks! This type of local control requires
    learning something different, and a cognitive “reprogramming”. In my
    opinion, this kind of local control is globally more efficient.

The algorithm developed in the current version of RawTherapee is close
in its principle to the U-points described above. Of course, the code
used in Nik Software is not disclosed, but several years ago I was
attracted by the simplicity of use and the efficiency of U-points, and I
started developing a product which would use neither lassos, nor layers
or fusion masks. Of course, nothing prevents us from having all 3 types
of local control in the same program.

The current version of the filter is perfectible, particularly regarding
the user interface (GUI):

- for managing and viewing the RT-spots (control spots) on screen,
- for manipulating the different sliders, which would ideally be better
  if integrated to the actual control points, as done in Nik Software.

However, those two aspects don’t harm everyday use, nor code
portability.

In order to improve manipulation and avoid long menus on screen, I added
a GUI interface which is similar to the one used in the Wavelet tab,
with “expanders”. Thus for each module (“Color and Light”, “Blur”,
“Retinex”, …) you can choose to:

1.  display (or not) the full list of commands: sliders methods, curves…
2.  activate (or not) all the functions of the module.

Warning: The second choice activates or cancels all the RT-spots. (It
would be possible in theory to add this capability to every single
RT-spot, but for what purpose?)

### Lab and RGB local controls

- The first algorithms are coded in L\*a\*b\* mode with the following
  modules: “Color and Light”, “Blur / Add noise”, “Retinex”, “Tone
  mapping”, “Sharpen”, “Contrast by Detail Levels”, “Denoise”. Two
  additional modules have been added: “Exposure” and “Vibrance”.
- See below for the principles of the different algorithms, but one key
  point to remember for shape detection and artifact limitation, is that
  the Lab mode preserves the “hue” tint, which is crucial.
- The idea of an “RGB” module comes to one’s mind, with *a minima*:
  - a “White balance” module, which would allow to locally adjust the
    white balance, for example to warm up shadows, or to deal with mixed
    scene lighting such as “daylight” and “tungsten”. This module needs
    to be inserted just after “demosaic”.

#### Auto White balance

The “White balance” module (in the “autowblocal” branch) serves two
purposes:

- allowing a local adjustment of the white balance (of course)
- testing new algorithms for automatic white balance adjustment:
  - in general use (full image), in order to achieve better results than
    with the current algorithm. Of course one will tell me that I should
    create a specific branch for “White balance”, but here I’m trying to
    hit two targets with one bullet.

As a reminder, I’ve been asked in 2012 and 2013 to develop an iterative
white balance “Auto Robust
WB[1](http://ieeexplore.ieee.org/document/1649677/)”. At that time,
another developer and I had spent a lot of time on this for an
unsatisfactory result, so we gave up. Since then, benefiting from this
experience, I resumed this work but in the reverse way! Now, if I’m
correct, the algorithm works. I took advantage of this new capability
(currently working on raw images only) to search in the academic
literature for other auto WB algorithms. I found, and developed two new
algorithms:

- “auto
  edge”[2](https://www.researchgate.net/publication/301419990_A_Method_to_Improve_Robustness_of_the_Gray_World_Algorithm):
  the idea is to create an accentuation mask allowing to reinforce parts
  of the image where details are more prevalent, compared to flatter
  regions.
- “auto standard
  deviation”[3](http://www.acad.bg/rismim/itc/sub/archiv/Paper3_1_2012.pdf):
  here, the image (or part of it for the local control) is divided in 12
  zones, for each of which mean and standard deviation are computed, and
  the result assembled.

A third algorithm, “Color by
correlation”[4](https://courses.cs.washington.edu/courses/cse467/08au/labs/l5/whiteBalance.pdf),
looks promising but I’m facing several difficulties. After many trials
of many types, the results are just too erratic…

I also added the notion of gamma:

- the same principles which deal with luminance distribution – gamma
  sRGB – used in the main RawTherapee code.

Bear in mind that the automatic white balancing is a totally non
deterministic mathematical problem. The algorithms may perform more or
less well (and faster or slower), but a perfect result can’t exist! Why
is it so? If one is referring to the principles of color management
([5](http://rawpedia.rawtherapee.com/Color_Management_addon/fr#Balance_des_blancs)),
it is necessary to have knowledge of the three of: the colored object,
the illuminant and the observer. At least two of those are unknown in
practice. In addition, the environment in which an image is shot is not
known while it has an effect on the perception (CIECAM), and the same is
true during both image editing and viewing. The mission is thus almost
impossible!

The aim of the “White balance” module is **for testing the algorithms**,
there are 7 of them at least, and twice that number if “gamma sRGB” is
considered. Indeed, in order to circumvent the obstacles encountered
five years ago, instead of the using the image before demosaicing, I
used the image just after demosaicing, allowing to apply (or not) “gamma
sRGB”, ending in the same conditions as in normal use (but without
normal “White balance” and “ICC or DCP” profile).

Ideally, we should test the algorithms on many images, as well: which
algorithm(s) to keep and implement in normal – global RGB – mode (maybe
one or two, in addition to the current algorithm which we should keep
for compatibility).

#### Local control of White balance

The local control is currently limited to 1 spot only, but there’s no
difficulty to increase this if users want so… while waiting for the
evolution of “locallab”.

Of course, I hear voices say “it doesn’t work”, or “the output doesn’t
match with the preview”. But it does work! And it is more than evident
that matching exactly the preview is by definition almost impossible…
(differences between shadows, sun light, different illuminants, etc…)
whatever the relevance of the algorithm.

## What are the challenges we need to solve?

Several general issues need to be solved so as to achieve smoothness in
use:

- allow an (almost) unlimited number of RT-spots (the name we chose for
  the control spots in RawTherapee);
- adapt the local algorithms to be aware of scale, because many of them
  take the image size – the treated area – into account. This aspect is
  fundamental, particularly with curves which act on the whole image;
- minimise memory use and computation time for JPG and TIFF output;
- allow easy updates in case of evolution of either the algorithms or
  the number of methods/modules;
- optimise (minimise) the differences between the preview and the
  output.
- etc…

For each RT-spot:

- allow for the action to occur, on user choice, either inside or
  outside (“Inverse” mode) of the selection area;
- allow shape detection, on user choice, in order to limit the action
  based on features/shapes;
- take care of the transition between the center of the treated area and
  the other parts of the image;
- etc…

Currently, RT-spots are operational in Lab mode with the following
modules and methods:

1.  “Color and Light”: “Lightness”, “Chrominance”, “Contrast” in normal
    mode with 4 curves L=f(L), L=f(H), C=f(C), H=f(H), and in “inverse”
    mode;
2.  “Exposure”: normal mode only;
3.  “Vibrance:” normal mode only;
4.  “Blur & noise”: normal and “inverse” modes;
5.  “Retinex”: normal and “inverse” modes, now also with “transmission
    gain” curve;
6.  “Sharpening”: normal and “inverse” modes;
7.  “Contrast by detail levels” (CBDL): normal mode only;
8.  “Denoise”: normal mode only.

## General principles

### The RT-spot object

As I mentioned earlier, the system used in RT is similar to the one
developed in Nik Software, but with some significant differences:

- each RT-spot can be viewed as an object with a number of fields (about
  70 as of today), made of sliders, curves, control points, etc…;
- each field, grouped in modules, can be activated or not, and have
  different parameter values according to their nature;
- the modules are made to be coherent for the user: “Color and Light”,
  “Exposure”, “Contrast”, “Vibrance”, “Blur”, “Retinex”, “Sharpening”,
  “Tone Mapping”, “Contrast by detail levels”, “Denoise”;
- the number of RT-spot objects can vary from 2 up to 500;
- data describing the RT-spot objects are stored in text files with .mip
  extension;
- creating, modifying and tracking of the RT-spot objects are done in
  a “for” loop;
- there’s no code duplication.

### Partitioning into 4 zones – previewing the RT-spot area

When the user selects an RT-spot, the screen preview shows:

- a centre, made of a circle whose diameter and position can be adjusted
  directly using the mouse or the sliders;
- two horizontal and two vertical handles, whose position can be
  modified directly using the mouse or the provided sliders, controlling
  the extent of the area affected by the RT-spot;
- if "Show spot delimiters" is checked in "Preferences" \> "General" tab
  \> "Local adjustments", one can visualise approximately the area
  affected by the spot. Because I couldn’t work cleanly with the
  « arc/ellipse/scale » function in the Cairo graphics library, I made
  each quadrant defined by a pseudo-ellipse made of 3 Bézier curves
  joining each other. In most cases the approximation is satisfying… To
  help grabbing the 4 handles, I made them longer (the Bézier curves are
  inactive!).

We obtain 4 zones, whose orientation cannot be modified (the default
rotation angle is set to zero). Those 4 zones have each of their
vertices connected by imaginary ellipses.

It is possible to drag the handles outside the image preview area.

It should be possible to replace the ellipse with a hand drawn curve
(even if I think its usefulness is debatable because of the existence of
the shape detection algorithm), as long as the homothetic transform of
the curve doesn’t intersect with the original curve. Although the
benefits from this intellectually satisfying “improvement” should
negligible in the case of the RT-spots, it will be possible to make the
current RT-spots and lasso type of controls coexist eventually.

### The "normal" and "excluding" types of RT-spots

Two types of RT-spots are available:

- "Normal": these are the spots which do the local retouching. Each new
  RT-spot takes into account the effect of existing spots when their
  zone of influence overlap (a sort of recursive action).
- "Excluding": adding a new "excluding" RT-spot allows to revert the
  effect created by a "normal" RT-spot in their overlapping zone of
  influence , from the original image data (but doing its calculations
  from the reference values of the area affected by the "normal"
  RT-spot).

For both types, the user has access to most of the modules ("Color and
Light", "Contrast by detail levels", "Blur", ...)

#### Comments about the "excluding" RT-spot

- The principle of the "excluding" spot is similar to the "counterpoint"
  in Capture NX2, and is useful when for example, the effect of a
  "normal" RT-spot bleeds into an area which the user wants to be
  unaffected. This unwanted effect happens when when the tints of the
  two areas (the area the user wants to be affected, and the area one
  wants to remain unaffected) are to close.
- The underlying algorithm is similar to the main algorithms used
  elsewhere in "locallab", and is based on tint differences. While it's
  not perfect it should give satisfaction 70% of the time.
- I implemented an additional algorithm (with no guarantee that it will
  actually work one day) to help discriminate the areas, based on a
  Sobel-Canny transform. The transform is coded but is not yet
  accessible to the user, as I'm still working to make it more
  efficient.

#### How to use the "excluding" RT-spot, and its limitations

1.  Create a new RT-spot over the area you want to restore.
2.  In the "Settings" panel, from the "Spot method" combobox choose
    "Excluding spot".
3.  Adjust "Scope", "Transition" and "Spot size" as well as the shape of
    the zone of influence using the 4 handles, until you get the desired
    reversion of the effect. You can use complimentary adjustments such
    as "Color and light", etc.

The algorithm computes the reference values (tint, chroma, luma) from
the the current image (see below). As a consequence, the "excluding"
effect won't work if the image area is black (for example when the user
uses a normal RT-spot using "Color and light" in inverse mode to create
a black frame). Sometimes, when the "inverse" mode is selected, the
"excluding" spot algorithm may not work as expected and lead to a
difference between the preview and the output (JPG, TIFF) image.
Increasing the value of "Scope" of the "excluding" RT-spot should help
in this situation.

### Tint, chroma, luminance references and the algorithm principle in “normal” mode

In order to develop an efficient shape detection algorithm:

- The central circle zone is used as the reference. Based on the
  diameter value chosen by the user, the algorithm computes the tint
  (hue), chroma and luminance averages.
- The choice of the central zone diameter depends on the use intent. For
  example, if working on a foliage area the user would benefit from
  choosing a smaller diameter value in order to select only the foliage
  green; in contrast, for skin tones the user should increase the
  diameter to avoid the detection of parasites (noise, eye lashes…).
- For each quadrant, depending on the the value of the “Scope” slider,
  the algorithm does:

1.  take into account the tint difference between the zone centre and
    the affected pixel;
2.  through a complex algorithm based on the deltaE notion (perceived
    difference between two colors, taking tint, chroma and luminance
    into account), decrease the amount of applied effect as a function
    of the difference between the central zone and the affected pixel;
3.  follow either a linear or a parabolic law to adjust the amount of
    applied effect, based on the specific the case.

This allows adapting the amount of effect according to the
above-mentioned criteria. For example, if the central circle is located
on foliage, the action will be limited to the leaves, without touching
the background (which would be impossible to obtain with the lasso type
of controls). Additionally, if some other foliage resides within the
area covered by the RT-spot, it will also be affected.

- Adjusting the value of the “Transition” slider modifies the transition
  of the effect (from the centre to the edge): on 50, the inner (linear)
  half will get the full 100% action/effect, while the effect in the
  outer half will gradually transition from 100% to 0% towards the edge;
- By increasing the value of “Scope”, progressively more of the selected
  area is taken into account, whatever the tint, chroma and luminance;
- Conversely, by decreasing the “Scope” value the effect will get
  limited to the pixels which are more similar (in terms of deltaE) to
  the reference zone.

The shape detection algorithm works in “Normal” mode except for some
modules (“Denoise”). It’s not implemented in “Inverse” mode, doing so
would not make sense.

The algorithm will see its performance increase when the user chooses
“Enhanced” or “Enhanced + chroma denoise” in the “Global quality:”
combobox. The second choice additionally applies a low amount of chroma
noise reduction in order to get rid of artefacts which could be brought
up by the “Enhanced” algorithm (note that the chroma denoise step
increases memory use and computing time).

The algorithm is used in full capacity for “scope” \< 20.

### Why no alternative algorithms?

It should be possible to implement other shape detection algorithms than
the deltaE-based one used currently, for example:

- edge detection algorithms such as “Canny” or “Sobel”;
- wavelet decomposition.

But in both cases, everything would have to be done!

### Functioning in “inverse” mode

When it is available, the “Inverse” mode is extremely simplified: only
the “Transition” slider can be used to apply the effect. As of late
October 2017, I devised a new method for “Inverse” mode. It is limited
to the “Blur & Noise” module (see the corresponding section below).

### Maximum number of RT-spots

By default the number of available RT-spots is 8. You can choose any
number between 2 and 499. For this you only need to change the parameter
“Nspot” in the “options” file (for example: Nspot=12). Restart
RawTherapee for the modification to be taken into account.

### The “super” algorithm

The key to “success” was the algorithm implemented in late January 2017,
which works this way:

1.  on the pixel data which are within the area covered by the RT-spot,
    the algorithm applies either a “traditional” curve, a slider, or a
    normal transfer function (“Retinex”, “Tone Mapping”, “Contrast by
    detail levels”, “Vibrance”, …);
2.  the LUT or the transfer function is converted into an equivalent of
    a slider, separating negative and positive values so that the
    function can be viewed as multiple sliders (as many sliders as there
    are pixels) in the -100, 0, +100 interval;
3.  the data are passed to the shape detection function – the 4
    quadrants. For every pixel, the difference in terms of tint, chroma
    and luma is computed with regards to the central reference. The
    linear variation equations are estimated for luminance (L=f(L)),
    chroma (C=f(C)) or transfer functions;
4.  different corrections are applied to account for the environment
    (sky, skin tones…)
5.  a correction is made (with threshold and iterations) in order to
    avoid correcting (or only slightly, based on the user’s choice) the
    grey or neutral tones which are in the same tint as the reference
    (but with a low chroma value);
6.  the parameters are weighted based on the shape of the RT-spot
    (ellipses) and the transition zone;
7.  the values of the “Global quality:” parameters are important,
    particularly so for images with a low amount of noise – chroma noise
    is interpreted by the algorithm being of the same color as the spot.
    It is thus necessary to suppress this noise, that is the purpose of
    “Enhanced + chroma denoise” (this algorithm may not be sufficient,
    though – in this case using the local “Denoise” module may be
    useful).

This new (“super”) algorithm seems to perform generally well, but in the
event when the RT-spot is located in a grey/neutral or flat area, the
algorithm may fail. In such cases, using the old algorithm is
recommended (except for “Retinex”, “Tone Mapping” and “Contrast by
detail levels” for which it is made unavailable for the sake of code
simplicity).

### Artefact reduction and the “Global quality” algorithms

By default, “Global quality:” is set to “Enhanced + chroma denoise”.
Whereas it increases computation time, it is still generally preferable.
In case one wants a more responsive preview, “Enhance” or “Standard” can
be chosen (good for exploring the image).

- two sliders allow reducing artefacts and improving the algorithm’s
  rendering. They work based on chroma, in neutral/gray tones. To
  completely remove its effect, one can simply set “iterations=0”. These
  sliders may be used for "Color light", "Exposure", "Retinex",
  "Vibrance", "Tone mapping" and "Contrast by detail levels";
- for (even slightly) noisy images, the algorithms may be misled. In
  this case it is recommended to keep "Global quality:” set to
  “Enhanced + chroma denoise", and (sometimes) activate the local
  “Denoise” module and adjust the sliders very gently (including
  “Luminance”);
- under some circumstances, with “Tone Mapping” for example, the
  artefact reduction algorithm may actually generate more artefacts.
  When it happens, set “iterations=0”.

If your images are generally clean (no or very low noise), you can
choose to have “Standard” as the default for “Global quality:” – this
will speed up the image processing. To do so, go to “Preferences” \>
“Performance and quality” tab \> “Local adjustments” and choose the
“Standard” algorithm among the 3 options. This will become the new
default when new RT-spots are created, but of course the algorithm can
still be changed on the fly by choosing from the “Global quality” combo
box.

### Avoid color shift

The “Avoid color shift” checkbox serves two purposes:

- put the colors within the current working profile gamut, using a
  relative colorimetry;
- adjust colors using a “Munsell” correction – particularly for
  red-orange and blue-purple colors, when saturation in the L\*a\*b\*
  space has changed significantly.

### “mip” files – Principles of the multi-point algorithm

For the local control system to work and keep track of RT-spot objects,
a text file – similar to a pp3 – is written in 2 possible places:

- next to the edited image file. If, for example, you edit an image
  named ASC4509.NEF, a new file named ASC4509.NEF.mip will be generated
  in the same folder. In this case, several sessions can be open for the
  same image, as long as copies of the image are stored in different
  folders. However, the folder names in the path have to contain ASCII
  characters only, otherwise it will make the program crash;
- in the dedicated “mip” folder, within the “cache” directory (through
  an MD5 hash number which identifies the file). This is the default. If
  chosen, when multiple copies of the same file exist in distinct
  folders, editing them creates as many “mip” files. This option allows
  the use of non ASCII characters in the folder names and path.

The choice between these two behaviours is made in “Preferences” \>
“Image processing” tab \> “Mip profiles”.

The “mip” files contain the data which are passed to RawTherapee through
processes of the “procparams” type, and LUTs which allow editing in zoom
mode. When updating RawTherapee to a new version, it may be required to
delete the “mip” files (or the whole cache) to avoid a potential crash
of the application. This can be achieved by going to “Preferences” \>
“File browser” tab, then choosing “Clear mip” and/or “Clear pp3” – all
this is valid only if “mip” and “pp3” files have been set to be saved to
the cache folder, otherwise they will have to be manually deleted from
the working folder. Keeping older “mip” and “pp3” files may lead to
instability or crashes of the application.

#### Architecture of the filter

When I “ventured” into a multi-point, no-layer and no-mask system, code
duplication had to be avoided. Indeed, it is unthinkable to consider,
for 10 RT-spots, generating 10 GUI code blocks, and just as many code
blocks in “rtengine”. After careful consideration, the idea was to have
a file updating and tracking in real time – i.e. a file which would
follow each modification made by the user – the actions/modifications
history.

Of course, the parameters used by RawTherapee for the GUI and for the
main code through “params” are of different types:

- numeric: integer, float, double,…
- Boolean
- string (in choice menus for the different modules)
- curves, through “vectors” of type double with dynamic dimension
- …

#### Data conversion

In order to simplify data management, the first step is to convert all
data to integers. Sometimes this can lead to a slight precision loss,
which I think is negligible.

- This operation is very simple for numeric values, even though in some
  cases (such as hue, which is expressed in radian in the interval
  \[-Pi, +Pi\]) it needs a double “float\*100 and /100 conversion to
  keep precision.
- It is logical for Boolean operations and methods.
- However, it is a complex matter regarding curves. The values to be
  recorded in the “mip” file are of type vector<double>, in a dynamic
  table of numeric values. There may be a simpler way, by using existing
  functions from “procparams”, but I didn’t succeed.

Nevertheless, I could get around this by:

1.  converting doubles to integers with 3 significant figures – which is
    largely enough,
2.  then converting the “vector” to a variable length character string,
3.  the writing and reading of this character string and converting to a
    vector dynamically using an appropriate function (”strcurv_data”).
    Note that this function allows adjusting the curves within the limit
    of 16 inflection points (Bézier curve) for the flat curve, and 32
    points for the diagonal, which should be enough. Whether this would
    not be enough, it should be possible to increase that number, though
    compatibility would be compromised.

#### Some elements about curves

The implementation of curves has been a complex task, particularly
since:

1.  each curve needs its own reset function – resize() at each
    iteration,
2.  which implies oddities regarding their description, for example the
    diagonal curve gets initialised with several points, despite it
    looks being “empty”. This is mandatory for correct functioning, but
    this means that the curve is always open/active. Consequently, to
    avoid some unwanted loss of computation time in “preview” mode, the
    user has to activate the curve by clicking on the “curves” combobox.
    The same activation is necessary for L=f(H) and C=f(C) curves. While
    clicking enabling the “curves” (linear) would seem to be a better
    solution, it didn’t work despite many trials.
3.  during all the procedure, we need to set each "params" value to
    "clear" for curves, for each LUT, at each iteration and then through
    a "vector" to reassign the good, updated values to "curve params".

#### Data storing and dynamic use

Once data have been converted:

- they are stored in a text (mip) file,
- each one is referenced inside a 2-dimension dynamical table:

1.  dataspot\[x\]\[y\] for numeric and boolean values and methods, where
    “x” is the representative index of the parameter (e.g. “9” is for
    “lightness”) and “y” represents the current RT-spot (control spot).
    Value “0” represents the current in-use value (the one stores in
    “params”).
2.  retistr\[y\], llstr\[y\], lhstr\[y\], ccstr\[y\], hhstr\[y\],
    skinstr\[y\], pthstr\[y\], exstr\[y\] for parameters of type
    “curve”. Currently there are only 8 parameters, used for local
    “Retinex”, “Color and light” (L=f(L), C=f(C), L=f(H), H=F(H)),
    “Vibrance” and “Exposure”, but it is easy to add more. “y” is used
    as above in 1.

The way data are stored and used fits well with improccoordinator.cc
(current management / preview) and simpleprocess.cc (TIFF / JPG output)
files, but not so with dcrop.cc and the partial display of the image
(RawTherapee’s pipeline).

In this case (zoom / preview) another solution had to be found in order
to synchronise the modifications in real time. I made use of LUTs, so
that to each entry referenced in the table described above a LUTi(z) is
associated, with z=500 possible entries (500 corresponds to the current
maximum number of RT-spots, but this limit can be decreased or
increased). The LUTi “z” corresponds to the dataspot “y”. A 25,000
entries LUTi (arbitrary number) is used for “retistr”, “llstr”, “lhstr”,
“ccstr”, “hhstr”, “skinstr”, “pthstr”, “exstr” for storing each “vector”
curve value in memory (up to 69 values stored as integers).

During data processing, exchanges occur between: params, dynamic tables
and LUTi. The values used by dcrop.cc are dynamically linked to
dynamical tables by parent→.

#### Exchange processes and dynamic data storing

1.  reading of the “mip” file to check the version number and guide the
    updates
2.  creating the “mip” file if it doesn’t exist
3.  storing the data in LUTi and dynamic tables from the values stored
    in “params” (index 0 values)
4.  first interactive update with the GUI through a transfer function
    between “rtengine” and locallab.cc (aloListener → localretChanged).
    This is one of the 3 functions required for the process to work
    correctly
5.  reading of the “mip” file and data storing in dynamic tables (index
    values ==\> y) according to the number of RT-spots
6.  creating a loop – according to the number of RT-spots – which
    updates LUTi (required by “dcrop”) and “params” values from values
    stored in the dynamic tables
7.  call to the “Lab_local” function which contains the algorithms
    controlling each RT-spot (luminance, sharpness, retinex, denoise, …)
8.  second interactive update with the GUI through another transfer
    function between “rtengine” and “locallab.cc”
9.  treatment of the current RT-spot by using the index (0) values from
    the dynamic tables. These values are attributed to 1) “params”, 2)
    LUTi and 3) current index values
10. call to the “Lab_local” function for the current RT-spots
11. saving to the “mip” file.

Note that if the user modifies the curve (amplitude, number of
inflection points) a third interactive update is made in order to
conform all of the data according to events.

#### A few unavoidable “fantasies”

Of course everything looks simple, the process works if, and only if,
all the data are dynamically updated. I’ve tried a lot, went groping,
and finally found a working solution… but an uncanny one. There’s
probably a more “informatically” correct way to do it, but at this point
I’ve done as explained above and hereafter.

The 3 exchanges between “rtengine” and the GUI occur with 3 parameters
which actually do nothing in the program except doing the update
process:

1.  “anbspot” which takes values of 0 or 1
2.  “cTgainshaperab” (curve) which takes any value bewteen 0.70 and 0.90
3.  “retrab” which varies around 500.

I have added 3 more parameters (in the form of sliders) – hueref,
chromaref and lumaref – hidden from the user but which are used to
update the system in real time, to correctly take into account the
reference “spot” values. Allowing those 3 parameters to vary brings
stability and allows real time updates. Increasing the number of those
“fantasy” parameters should not be necessary. These extra parameters are
hidden to the user, and are only visible in the history.

In addition, I had to add some empirical time delays, giving time to
time… Time delay is used for example when the user modifies the curve
(amplitude, number of inflection points).

## Specificity of the local mode (as compared to “Lab adjustments”)

Hereafter is some useful information for the user, often specific to the
local mode.

### Color and Light

- The algorithms used for luminance and contrast in local mode differ
  from the ones used in “Lab adjustments”, which may lead to differences
  in rendering.
- The luminance algorithm is made such that below -90 and down to -100
  for “Lightness”, it is possible to increase the image's “darkness”,
  almost to the point of getting a luminance value close to 0. To
  accentuate this effect, “Chrominance” can be decreased and “Scope”
  increased.
- The inverse mode can be used for creating graduated frames. If
  “Ligthness” is set to -100 and "Chrominance" is reduced, the “frame
  border” will be black.
- For the “Lightness” and “Contrast” sliders, the user can choose among
  2 algorithms: the first one (default) is close to that in “Lab
  adjustments” while the second one, activated through “Lightness -
  Contrast ‘Super’” is close to the algorithm used for the L=f(L)
  “Super” curve.
- Do not hesitate, when needed, to activate “Enhanced” or “Enhanced +
  chroma denoise” in the “Global Quality:” combobox.

#### Curves

- An L=f(L) or C=f(C) curves allow controlling the luminance and
  chrominance for each RT-spot as a function of luminance or
  chrominance.
- An L=f(H) curve allows controlling the luminance for each RT-spot as a
  function of tint.
- An H=f(H) curve allows controlling the tint for each RT-spot as a
  function of tint.

The curves are activated by selecting one of the two algorithms from the
“Curves types” combobox:

- “Normal”: the curves – particularly the one that is the most difficult
  to manage, L=f(L) – use an algorithm close to the one used with the
  sliders (Normal mode),
- “Super”: using a new algorithm, which I think performs very well (it
  even surprised me the first time I used it).

Caution: when the RT-spot resides in an area which is neutral, grey or
very uniform, the artefacts introduced by the new algorithm (“Super”
algorithm for the curve or the slider) may be impossible to get rid of.
Use one of the 2 older algorithms to avoid that.

### Exposure

The “Exposure” module may look like the the one in global RGB mode, but:

- it works entirely in L\*a\*b\* mode, hence the differences in
  rendering;
- there’s no “Lightness”, “Chroma” or “Contrast” slider, whose functions
  are already fulfilled in the “Color and Light” module;
- there’s only one “Contrast” curve, similar to the L=f(L) firm “Color
  and Light”. Of course its effect is different from “Tone curve” which
  works in RGB mode. If needed, two L=f(L) curves can be activated, one
  under “Color and Light” and the other under “Exposure”.

### Vibrance

This module is similar to the main one (from RawTherapee's "Exposure"
tab).

### Blur & Noise

“Blur” is activated only when “Radius” =\< 2. By significantly lowering
the default value of “Scope” and possibly checking “Blur luminance
only”, it is possible to get a different amount of blur for the
different tints. Since October 2017, 3 methods are proposed:

- “Normal”: the shape detection algorithm has been improved, it is now
  closer to shape detection algorithm proposed in the other modules;
- “Inverse”: a simplified algorithm, which doesn’t make use of “Scope”.
  The inversion is coarse and doesn’t allow a fine separation between
  the affected and unaffected areas;
- “Symmetric”: this new algorithm uses the same processes as “Normal”,
  and adjusting “Scope” the user can target the process with more
  precision.

Caution:

- Using the “Inverse” mode will blur large portions of the image, which
  can not be corrected later on.
- The action of “Scope” may sometimes appear puzzling: for instance, in
  a portrait, the eyes (which of course differ from the skin) may be
  blurred if the value of “Scope” is too low.

### Tone Mapping

Caution when uniformly colored areas (sky, grey/neutral zones) have a
similar tint to the area on which the RT-spot is placed. In cases where
this leads to artefacts, set the “Iterations” slider to 0 in “Settings”
\> “Reduce artefacts – Improve algorithm”.

### Retinex

Compared to the standard “Retinex” module, the local “Retinex module has
simplified controls, and is applied at the end of the pipeline instead
of the beginning. Since “mip” version 10002, the “Transmission gain” is
specific to each RT-spot.

### Sharpening

Only the “RL Deconvolution” algorithm is provided here.

### Contrast by detail levels

Compared to the main RGB mode module, this one has:

- 5 levels instead of 6,
- no slider for “skin tone” protection (not needed since the algorithm
  works locally on the RT-spot area),
- an additional “Chroma” slider.

Caution when uniformly colored areas (sky, grey/neutral zones) have a
similar tint to the area on which the RT-spot is placed. In cases where
this leads to artefacts, set the “Iterations” slider to 0 in “Settings”
\> “Reduce artefacts – Improve algorithm”.

### Denoise

Compared to the main RGB mode module, this one has:

- only the wavelet tool working (no Fourier function, nor medians),
- less sliders and curves,
- the possibility to work differentially on fine or coarse noise for
  both luminance and chrominance noise.

## A specific use case : reduction of image defects (dirty sensor, red eyes, …)

“Locallab” had not been thought with the idea of correcting small image
defects. However, some visible, “photographically disturbing” defects
may actually be tamed or even removed. Of course, this is not
incompatible with other more specialised modules which could be
introduced (or not) into “Locallab”… At least 3 existing modules can
serve this purpose, alone or in combination, by adapting their use:

- “Color and Light” for spot-like defects;
- “Contrast by detail levels” for spread out defects;
- “Blur and Noise” as an optional complement (caution, this is a
  destructive action, use moderately).

These modules can be used either on the main image (raw, TIFF, …) or on
a flat-field image.

### Using the “Color and Light” module

- Activate “Color and Light”
- A red eye removal tool equivalent can be obtained by framing closely
  around the eye – central circle zone centered on the eye’s red, spot
  handles close to the eye – low “Scope” value, then lowering
  “Lightness” and “Chrominance” to -100.
- The same idea allows reducing the “IR spot” kind of defect by framing
  closely around the defect – central circle zone centred on the defect,
  spot handles close to the defect area – then lowering “Chrominance” to
  taste, and if needed lowering the “scope” value.
- In some (simple) situations, a “dirty sensor” defect can be reduced,
  in particular in the case of “sensor grease”. It can be seen as stains
  or spots on a uniform background. To attenuate this, choose a tight
  framing of around the defect – central circle on the centre of the
  defect (adapt the size of the RT-spot), drag the handles so that they
  are not too close to the border of the defect (this will make the
  transition less noticeable). Then: a) decrease “Transition” towards
  lower values; b) check “Lightness - Contrast “Super”; c) adjust
  “Lightness” and “Chrominance” as needed to bring the defect area
  closer to the unaffected area; d) if needed adjust slightly “Scope” to
  achieve the best result.

### Using the “Contrast by detail levels” module

When the sensor is dirty (grease), and when the affected area is large
or if there are multiple small defects, “Contrast by detail levels” may
be used, working as a sort of “wavelet” tool on luminance, and if needed
on chrominance. In such cases: a) place the RT-spot on one of the
stronger defects (adjust the size as needed); b) drag the handles so as
to cover most of the affected area; c) set “Transition” to higher value;
d) activate “Contrast by detail levels” and decrease the contrast of
levels 3 and 4 (or lower); you can adjust the “Chroma” slider if needed.

## Settings in the Preferences dialog

Below is a list of the settings (already mentioned above in several
places) which can be accessed from the “Preferences” dialog:

- In “Preferences” \> “General” tab \> “Local adjustments”, by checking
  the “Show spot delimiters” checkbox, an approximate zone delimitation
  is drawn which helps viewing the area affected by the RT-spot and its
  settings.
- If your images generally have low noise, you can set the default for
  “Global quality” to “Standard”, and get a significant speedup in
  processing time. This setting is found under “Preferences” \>
  “Performance & Quality” tab \> “Local adjustments”. The three choices
  for quality are selected from the “Local adjustments Quality method”
  combobox. Default is “Enhanced + chroma denoise”.
- Regarding “mip” files:
  - the choice for “mip” files destination folder is set from
    “Preferences” \> “Image processing” tab \> “Mip Profiles”;
  - in order to clean the generated “mip” profiles, go to “Preferences”
    \> “File Browser” tab \> “Cache Options” and click on the “Clear
    mip” button and/or “Clear pp3” – this is valid in the case where you
    chose to store “mip” files in the cache, otherwise if you chose to
    let RT write the “mip” files next to the input file, you will have
    to delete them manually. Note that the presence of older versions of
    “mip” and “pp3” files may lead to instability or even crashes of
    RawTherapee.

## Processing time and memory use

At image export time (output to JPG, TIFF…), for the “normal” mode, the
algorithms compute the data only for in the RT-spot delimited areas, not
for the whole image. Thus, processing time and memory stay reasonable.
Note that of course, this will depend on the number of RT-spots, their
size, and the type(s) of treatment done in these RT-spots.

Excluding “Denoise” and “Sharpen”, processing time is in the order of a
few tenths of a second per RT-spot, and memory use of a few hundred MB.

Two settings have a strong influence on resource use:

- “Denoise” which adds around 1 sec. of processing time, and 1 MB of
  memory use;
- “Global quality: Enhanced + chroma denoise” which adds around 0.5 sec.
  And 0.6 MB.

These figures depend on the size of the RT-spot.

## Future evolution

Besides usual tweaking, bug corrections, improvements (settings, user
interface)… it would be desirable to improve the GUI with regards to the
creation/selection/modification of individual RT-spots.

From a programming point of view, it should be possible to improve code
readability and maintainability, and to integrate all the “mip” files
code parts (as done in “rgbproc.cc”) which currently are linearly
integrated to void procedures in “improccoordinator.cc”, “dcrop.cc” and
“simpleprocess.cc”.

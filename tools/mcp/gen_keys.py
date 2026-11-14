#!/usr/bin/env python3
"""Scan src/engine/procparams.cc for the (group, key) pairs that the .arp
reader/writer know about and write known_keys.json next to this script.

The server uses it to reject typos in LLM-written profiles. Re-run after the
profile format changes:  python3 gen_keys.py [path/to/procparams.cc]
"""
import json, os, re, sys, collections

here = os.path.dirname(os.path.abspath(__file__))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    here, "..", "..", "src", "engine", "procparams.cc")
text = open(src, encoding="utf-8").read()

# `f("Group", "Key"` where f is one of the keyfile helpers; whitespace/newlines
# between the arguments are allowed (the code is clang-formatted)
call = re.compile(
    r'\b(?:saveToKeyfile|assignFromKeyfile|loadFromKeyfile|has_key|has_group|'
    r'get_\w+|set_\w+|APPEND_\w*|assign\w*|load\w*|save\w*)\s*\(\s*'
    r'(?:[\w.>-]+\s*,\s*)?'          # optional leading argument (e.g. keyFile)
    r'"([^"\n]+)"\s*,\s*"([^"\n]+)"')
keys = collections.OrderedDict()
for m in call.finditer(text):
    g = re.sub(r"\s+", " ", m.group(1)).strip()
    k = m.group(2).strip()
    keys.setdefault(g, [])
    if k not in keys[g]:
        keys[g].append(k)

# keys that only old profiles carry (migration code), commented-out code that
# the regex still sees, and indexed keys that are spelled out explicitly
drop_groups = ["HLRecovery", "Directional Pyramid Denoising"]
drop_keys = {
    "Exposure": ["CurveMode", "CurveMode2", "Curve", "Curve2", "HistogramMatching",
                 "CurveFromHistogramMatching", "Saturation"],
    "RAW": ["DCBIterations", "DCBEnhance", "PreBlackone", "PreBlacktwo",
            "PreBlackthree"],
    "RAW Bayer": ["DCBIterations"],
    "ToneEqualizer": ["Band"],
}
for g in drop_groups:
    keys.pop(g, None)
for g, ks in drop_keys.items():
    if g in keys:
        keys[g] = [k for k in keys[g] if k not in ks]
keys["ToneEqualizer"] = keys.get("ToneEqualizer", []) + ["Band%d" % i for i in range(5)]

# groups whose keys are generated at run time (indexed regions, masks, curves
# per snapshot, ...): extra keys are accepted for these
dynamic = ["ColorCorrection", "Smoothing", "Local Contrast", "TextureBoost",
           "Spot Removal"]
out = {"groups": keys, "dynamic": [d for d in dynamic if d in keys]}
json.dump(out, open(os.path.join(here, "known_keys.json"), "w"), indent=1)
print(len(keys), "groups,", sum(len(v) for v in keys.values()), "keys")

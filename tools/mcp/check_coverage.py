#!/usr/bin/env python3
"""Report the groups/keys of known_keys.json that the reference/ files do not
mention (a key counts as documented if `Key` appears in a backtick span inside
the section whose heading contains the group name)."""
import json, os, re, sys
here = os.path.dirname(os.path.abspath(__file__))
known = json.load(open(os.path.join(here, "known_keys.json")))["groups"]
ref = os.path.join(here, "reference")
text = "\n".join(open(os.path.join(ref, f), encoding="utf-8").read()
                 for f in sorted(os.listdir(ref)) if f.endswith(".md") and f != "FORMAT.md")
secs = re.split(r"(?m)^(?=#{1,3} )", text)
def norm(s): return re.sub(r"[\s_\-]+", "", s).lower()
missing = 0
for g, keys in known.items():
    body = "\n".join(s for s in secs if norm(g) in norm(s.split("\n", 1)[0]))
    if not body:
        print("NO SECTION:", g); missing += 1; continue
    miss = [k for k in keys if not re.search(r"`[^`\n]*\b%s\b[^`\n]*`" % re.escape(k), body)]
    if miss:
        print("[%s] undocumented: %s" % (g, ", ".join(miss))); missing += len(miss)
print("missing:", missing)
sys.exit(1 if missing else 0)

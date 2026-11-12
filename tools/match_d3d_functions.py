#!/usr/bin/env python3
"""Find WWE 2K16's XDK Direct3D functions by instruction-sequence similarity with a port that names them.

Same method as WWE 2K15's port/tools/match_d3d_functions.py: take the D3D functions named in a reference
port's names header, then rank the 2K16 functions of a code window (around the only VdSwap caller) by difflib
similarity of their PowerPC mnemonics. Nothing is written.

    python -I tools/asmindex.py build                                    # work/asm.json   (generated/)
    python -I tools/match_d3d_functions.py REF_REPO 82C10000 82C48000    # base game
    python -I tools/match_d3d_functions.py REF_REPO 82C10000 82C60000 work/asm_tu.json   # title update

REF_REPO is a local checkout of the WWE '12 port (it needs port/src/native/wwe12_d3d_names.h and the generated
code in port/generated/default/). work/asm_tu.json is the same index built from generated_tu/.
"""
import difflib
import glob
import json
import re
import sys
from pathlib import Path

ref_repo = Path(sys.argv[1])
lo, hi = int(sys.argv[2], 16), int(sys.argv[3], 16)
asm_path = Path(sys.argv[4]) if len(sys.argv) > 4 else Path(__file__).resolve().parent.parent / "work" / "asm.json"

names = {}
for line in open(ref_repo / "port/src/native/wwe12_d3d_names.h", encoding="utf-8", errors="replace"):
    m = re.match(r"#define (D3D\w+)\s+sub_([0-9A-F]{8})", line)
    if m and not m.group(1).startswith("__imp"):
        names[m.group(2)] = m.group(1)

ref = {}
for f in glob.glob(str(ref_repo / "port/generated/default/*.cpp")):
    text = open(f, encoding="utf-8", errors="replace").read()
    for m in re.finditer(r"^DEFINE_REX_FUNC\(sub_([0-9A-F]{8})\) \{\n(.*?)^\}", text, re.S | re.M):
        if m.group(1) in names:
            ref[m.group(1)] = [l.split()[0] for l in
                               re.findall(r"^\t// ([a-z][a-z0-9.+-]*(?: .*)?)$", m.group(2), re.M)]
print(len(names), "named in the reference,", len(ref), "found in its generated code")

target = {}
for key, instructions in json.load(open(asm_path)).items():
    addr = int(key.replace("sub_", ""), 16)
    if lo <= addr < hi:
        target[addr] = [text.split()[0] for _, text in instructions]
print(len(target), "functions in the window")

for a, ops in sorted(ref.items(), key=lambda x: names[x[0]]):
    scores = []
    for b, ops2 in target.items():
        if abs(len(ops2) - len(ops)) > max(20, len(ops)):
            continue
        sm = difflib.SequenceMatcher(None, ops, ops2, autojunk=False)
        if sm.real_quick_ratio() < 0.5:
            continue
        scores.append((sm.ratio(), b, len(ops2)))
    scores.sort(reverse=True)
    print(f"{names[a]:46s} ref sub_{a} ({len(ops):3d} ins) -> " +
          ", ".join(f"sub_{b:08X} {r:.2f} ({n})" for r, b, n in scores[:3]))

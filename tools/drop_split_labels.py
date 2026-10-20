"""Removes [functions] entries that split a real function: an entry lying between the two ends of an
"Unresolved (conditional) branch to T from F" in a codegen log is a label inside a function, not a function.

    python -I tools/drop_split_labels.py logs/codegen-<time>.log config/functions_fptrs.toml
"""
import re
import sys

log, config = sys.argv[1], sys.argv[2]
branches = [(int(t, 16), int(f, 16)) for t, f in
            re.findall(r"Unresolved (?:conditional )?branch to 0x([0-9A-F]+) from 0x([0-9A-F]+)", open(log).read())]
lines = open(config, encoding="utf-8").read().splitlines()
drop = set()
for line in lines:
    m = re.match(r"\s*(0x[0-9A-Fa-f]+)\s*=", line)
    if m:
        e = int(m.group(1), 16)
        if any(min(t, f) < e <= max(t, f) for t, f in branches):
            drop.add(m.group(1))
kept = [l for l in lines if not (re.match(r"\s*(0x[0-9A-Fa-f]+)\s*=", l) and
                                 re.match(r"\s*(0x[0-9A-Fa-f]+)", l).group(1) in drop)]
open(config, "w", encoding="utf-8").write("\n".join(kept) + "\n")
print(f"{len(branches)} unresolved branches, dropped {len(drop)} entries")

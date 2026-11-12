"""Merge pipelines.bin files of the native renderer (header: 4 u32 'W12P', version, sizeof(PipelineState), count; then the
raw states) into one seed file, dropping duplicates. Usage: merge_pso.py OUT IN [IN...]"""
import struct, sys

out_path, *inputs = sys.argv[1:]
magic = version = size = None
seen, states = set(), []
for path in inputs:
    data = open(path, "rb").read()
    m, v, s, n = struct.unpack_from("<4I", data, 0)
    if magic is None:
        magic, version, size = m, v, s
    elif (m, v, s) != (magic, version, size):
        print("skip (different layout):", path)
        continue
    body = data[16:]
    assert len(body) == s * n, (path, len(body), s, n)
    added = 0
    for i in range(n):
        st = body[i * s:(i + 1) * s]
        if st not in seen:
            seen.add(st)
            states.append(st)
            added += 1
    print(f"{path}: {n} states, {added} new")
with open(out_path, "wb") as f:
    f.write(struct.pack("<4I", magic, version, size, len(states)))
    for st in states:
        f.write(st)
print("wrote", out_path, len(states), "states")

"""Finds virtual-call thunks the codegen did not register: lwz rA,0(r3); lwz rB,N(rA); mtctr rB; bctr, starting
right after blr / bctr / padding so they are standalone functions, not the tail of a larger one.
These are only reached through pointer tables the game builds at run time, so the pointer scan can't see them.

    python -I tools/find_thunks.py work/image.bin generated/wwe2k16_init.h > config/functions_thunks.toml
"""
import re
import struct
import sys

image = open(sys.argv[1], "rb").read()
registered = {int(a, 16) for a in re.findall(r"sub_([0-9A-F]{8})", open(sys.argv[2]).read())}
BASE = 0x82000000


def word(a):
    return struct.unpack_from(">I", image, a - BASE)[0]


def is_terminator(w):
    # blr, bctr or padding. A plain `b` is not enough: inside a function it often just jumps over an else-branch,
    # and the code after it still belongs to that function.
    return w in (0x4E800020, 0x4E800420, 0)


found = []
for off in range(4, len(image) - 16, 4):
    w0, w1, w2, w3 = struct.unpack_from(">4I", image, off)
    # lwz rA,0(r3)
    if (w0 >> 26) != 32 or (w0 >> 16) & 0x1F != 3 or w0 & 0xFFFF != 0:
        continue
    ra = (w0 >> 21) & 0x1F
    # lwz rB,N(rA) ; mtctr rB ; bctr
    if (w1 >> 26) != 32 or (w1 >> 16) & 0x1F != ra:
        continue
    rb = (w1 >> 21) & 0x1F
    if w2 != (0x7C0903A6 | (rb << 21)) or w3 != 0x4E800420:
        continue
    a = BASE + off
    if a in registered or not is_terminator(word(a - 4)):
        continue
    found.append(a)

print("# Virtual-call thunks (lwz rA,0(r3); lwz rB,N(rA); mtctr rB; bctr) reached only through pointer tables")
print("# built at run time. Found by tools/find_thunks.py after a terminator; registered functions left out.")
print()
print("[functions]")
for a in found:
    print(f'0x{a:08X} = {{ size = 0x10 }}')
print(f"{len(found)} thunks", file=sys.stderr)

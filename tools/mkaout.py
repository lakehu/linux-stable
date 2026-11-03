#!/usr/bin/env python3
"""
mkaout.py RAW OUT - wrap a raw binary linked at address 0 into a Linux 0.11
a.out ZMAGIC executable (include/a.out.h): 32-byte header, padded to 1024
bytes (N_TXTOFF), then the text, padded to a 4096-byte page. No data, bss,
symbols or relocations; entry point 0.
"""
import struct
import sys

ZMAGIC = 0o413
raw = open(sys.argv[1], "rb").read()
text = raw.ljust((len(raw) + 4095) // 4096 * 4096, b"\0")
header = struct.pack("<8I", ZMAGIC, len(text), 0, 0, 0, 0, 0, 0)
open(sys.argv[2], "wb").write(header.ljust(1024, b"\0") + text)

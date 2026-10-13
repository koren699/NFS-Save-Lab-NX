#!/usr/bin/env python3
"""Independent Python HMAC fixture for game-tested Xbox Underground 2.
Only synthetic saves are generated; never commit the real career.
"""
import hashlib
import hmac
from pathlib import Path
import struct
import subprocess
import tempfile

SIZE=53868
CASH_OFFSET=0xA19A
def sign(data):
    assert len(data)==SIZE
    for at,start,stop in ((0x10,0x4c,0x54),
                          (0x24,0x54,SIZE),
                          (0x38,0,0x38)):
        data[at:at+20]=hmac.digest(bytes(16),data[start:stop],"sha1")
    return bytes(data)
def fixture(money):
    b=bytearray(SIZE)
    b[:4]=b"20CM"
    struct.pack_into("<III",b,4,SIZE,8,0xD218)
    struct.pack_into("<I",b,CASH_OFFSET,money)
    b[0x300:0x308]=b"U2SAMPLE"
    return sign(b)

with tempfile.TemporaryDirectory(prefix="nfslab_u2_") as td:
    original=Path(td)/"U2_ORIGINAL"
    fixed=Path(td)/"U2_100000"
    original.write_bytes(fixture(125))
    fixed.write_bytes(fixture(100000))
    subprocess.run(["./tests/test_u2",str(original),str(fixed)],check=True)
print("PASS: independent Python HMACs match native U2 writer with source protection")

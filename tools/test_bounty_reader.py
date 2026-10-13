#!/usr/bin/env python3
"""Synthetic MC02 fixtures exercising moving Bounty record slots."""
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
from inspect_mc02 import ea_crc32

MARK=bytes.fromhex("471cb2908be4e25547330031")
N=62688

def fixture(bounty,offset):
    out=bytearray(N)
    out[:4]=b"MC02"
    struct.pack_into(">III",out,4,N,8,N-0x24)
    out[0x1c:0x24]=b"MC02TEST"
    struct.pack_into(">I",out,0x4039,900)
    out[offset-12:offset]=MARK
    struct.pack_into(">f",out,offset,float(bounty))
    struct.pack_into(">I",out,0x10,ea_crc32(out[0x1c:0x24]))
    out[-16:]=hashlib.md5(out[0x34:-16]).digest()
    struct.pack_into(">I",out,0x14,ea_crc32(out[0x24:]))
    struct.pack_into(">I",out,0x18,ea_crc32(out[:0x18]))
    return bytes(out)

with tempfile.TemporaryDirectory(prefix="nfslab_bounty_") as temp:
    a=Path(temp)/"bounty31550.mc02"
    b=Path(temp)/"bounty45150.mc02"
    a.write_bytes(fixture(31550,0x10e))
    b.write_bytes(fixture(45150,0x15a))
    subprocess.run(["./tests/test_bounty",str(a),str(b)],check=True)

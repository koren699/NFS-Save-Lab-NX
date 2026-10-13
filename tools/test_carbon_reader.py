#!/usr/bin/env python3
"""Construct synthetic Carbon 20CM saves containing JDLZ-v2 literal payloads.
No private player saves are copied into the public repository.
"""
import struct
import subprocess
import tempfile
from pathlib import Path

TOTAL=262236
START=0x34bc0
UNPACK=32768

def literal_jdlz(source):
    assert len(source)==UNPACK
    out=bytearray(b"JDLZ"+bytes((2,16,0,0))+struct.pack("<II",UNPACK,0))
    out.extend((0,0))
    for i,value in enumerate(source):
        if i and not i%8:out.append(0)
        out.append(value)
    struct.pack_into("<I",out,12,len(out))
    return bytes(out)

def profile(money):
    uncompressed=bytearray(UNPACK)
    record=(b"2287685084:\t138\t0\t2\t0\t1\t33\t"+
            str(money).encode()+b"\t0\t0\n")
    uncompressed[:len(record)]=record
    compressed=literal_jdlz(uncompressed)
    assert START+len(compressed)<TOTAL
    out=bytearray(TOTAL)
    out[:4]=b"20CM"
    struct.pack_into("<I",out,4,TOTAL)
    out[START:START+len(compressed)]=compressed
    return bytes(out)

with tempfile.TemporaryDirectory(prefix="carbon_synthetic_") as tmp:
    first=Path(tmp)/"synthetic_5000.bin"
    later=Path(tmp)/"synthetic_10200.bin"
    first.write_bytes(profile(5000))
    later.write_bytes(profile(10200))
    subprocess.run(["./tests/test_carbon",str(first),str(later)],check=True)
print("PASS: synthetic 20CM/JDLZ Carbon Cash read + fail-closed validation")

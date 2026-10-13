#!/usr/bin/env python3
"""Construct synthetic Carbon 20CM saves containing JDLZ-v2 literal payloads.
No private player saves are copied into the public repository.
"""
import struct
import hmac
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
    # Real Carbon 10,200 save has 3 NONZERO JDLZ alignment bytes
    # immediately after its 3,269-byte compressed block.
    aligned=(len(compressed)+3)&~3
    for i in range(len(compressed),aligned):
        out[START+i]=b"\xce\xd5\xc4"[i-len(compressed)]
    out[0x34ba8:0x34bac] = bytes.fromhex("22114455")
    struct.pack_into("<I",out,0x34bac,UNPACK)
    aligned=(len(compressed)+3)&~3
    struct.pack_into("<I",out,0x34bb0,aligned+0x18)
    struct.pack_into("<I",out,0x4c,aligned+0x4c)
    # Game-tested Carbon Original Xbox save authentication:
    # three SHA1 HMAC tags (each 20 bytes), all-zero 16-byte key.
    struct.pack_into("<II",out,8,16,0x40000)
    key=bytes(16)
    for off,start,stop in ((0x10,0x4c,0x5c),(0x24,0x5c,TOTAL),(0x38,0,0x38)):
        out[off:off+20]=hmac.digest(key,out[start:stop],"sha1")
    return bytes(out)

with tempfile.TemporaryDirectory(prefix="carbon_synthetic_") as tmp:
    first=Path(tmp)/"synthetic_5000.bin"
    later=Path(tmp)/"synthetic_10200.bin"
    first.write_bytes(profile(5000))
    later.write_bytes(profile(10200))
    subprocess.run(["./tests/test_carbon",str(first),str(later)],check=True)
    subprocess.run(["./tests/test_carbon_edit",str(later)],check=True)
print("PASS: synthetic 20CM/JDLZ Carbon Cash read + fail-closed validation")

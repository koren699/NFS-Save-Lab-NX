#!/usr/bin/env python3
"""Independent synthetic MC02 Bounty fixtures; no user's save in repository.
Runs native C aggregate and editor regression checks against Python hashes.
"""
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile
from inspect_mc02 import ea_crc32

N=62688
BASE=0xE2F5
STRIDE=0x38
SOLD=0xE86D

def checksum(data):
    struct.pack_into(">I",data,0x10,ea_crc32(data[0x1c:0x24]))
    data[-16:]=hashlib.md5(data[0x34:-16]).digest()
    struct.pack_into(">I",data,0x14,ea_crc32(data[0x24:]))
    struct.pack_into(">I",data,0x18,ea_crc32(data[:0x18]))
    return bytes(data)

def fixture(retire_first=False, sold=0, second=9000):
    out=bytearray(N)
    out[:4]=b"MC02"
    struct.pack_into(">III",out,4,N,8,N-0x24)
    out[0x1c:0x24]=b"MC02TEST"
    struct.pack_into(">I",out,0x4039,500000)
    struct.pack_into(">I",out,0x4034,0x52)
    for i in range(25):
        at=BASE+i*STRIDE
        out[at]=0xff
    for i,(value,active) in enumerate(((22550,not retire_first),(second,True))):
        at=BASE+i*STRIDE
        out[at]=i if active else 0xff
        out[at+1:at+4]=bytes.fromhex("aa0300")
        out[at+7:at+12]=bytes.fromhex("000000aaaa")
        struct.pack_into(">I",out,at+0x10,value)
    for i,number in enumerate((0x51,0x52)):
        at=0x6221+i*0x14
        struct.pack_into(">I",out,at,number if (i!=0 or not retire_first) else 0xffffffff)
        out[at+0x11]=i if (i!=0 or not retire_first) else 0xff
    struct.pack_into(">I",out,SOLD,sold)
    return checksum(out)

two=fixture()
one=fixture(retire_first=True,sold=22550,second=22600)
expected=bytearray(one)
struct.pack_into(">I",expected,BASE+STRIDE+0x10,77450)
expected=checksum(expected)
assert len(two)==len(one)==len(expected)==N
assert struct.unpack_from(">I",one,SOLD)[0]==22550
assert struct.unpack_from(">I",one,BASE+0x10)[0]==22550 # stale retired
assert struct.unpack_from(">I",expected,BASE+STRIDE+0x10)[0]==77450

with tempfile.TemporaryDirectory(prefix="nfslab_bounty_") as temp:
    a=Path(temp)/"two-car.mc02"
    b=Path(temp)/"one-car-sold-history.mc02"
    c=Path(temp)/"expected-100k.mc02"
    a.write_bytes(two)
    b.write_bytes(one)
    c.write_bytes(expected)
    subprocess.run(["./tests/test_bounty",str(a),str(b),str(c)],check=True)
print("PASS: native per-car Bounty outputs match independent Python checksums")

#!/usr/bin/env python3
"""Synthetic test-only fixture. No actual user save is committed to GitHub."""
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import subprocess
TOOLS=Path(__file__).resolve().parents[1]/"tools"
sys.path.insert(0,str(TOOLS))
from inspect_mc02 import inspect,ea_crc32
from mc02_cash_test_copy import build_test_copy,validate

def make_sample(cash):
    b=bytearray(62688)
    b[:4]=b"MC02"
    struct.pack_into(">III",b,4,len(b),8,len(b)-0x24)
    b[0x1C:0x24]=b"MC02TEST"
    struct.pack_into(">I",b,0x4039,cash)
    struct.pack_into(">I",b,0x10,ea_crc32(b[0x1C:0x24]))
    b[-16:]=hashlib.md5(b[0x34:-16]).digest()
    struct.pack_into(">I",b,0x14,ea_crc32(b[0x24:]))
    struct.pack_into(">I",b,0x18,ea_crc32(b[:0x18]))
    return bytes(b)

original=make_sample(900)
assert validate(original)==900
patched=build_test_copy(original,900,14400)
assert validate(patched)==14400
assert validate(original)==900
assert original[0x4039:0x403D]==bytes.fromhex("00000384")
assert patched[0x4039:0x403D]==bytes.fromhex("00003840")
assert len(original)==len(patched)==62688
changed=[i for i,(a,b) in enumerate(zip(original,patched)) if a!=b]
assert all(any(start<=i<end for start,end in ((0x14,0x1c),(0x4039,0x403d),(62672,62688))) for i in changed)
for bad in (original[:0x403A]+b"x"+original[0x403B:],):
    try:build_test_copy(bad,900,14400);assert False,"checksum gate failed"
    except ValueError:pass
try:build_test_copy(original,901,14400);assert False,"wrong expected balance gate failed"
except ValueError:pass
try:build_test_copy(original,900,900);assert False,"no-op gate failed"
except ValueError:pass
with tempfile.TemporaryDirectory(prefix="mc02lab_") as d:
    source=Path(d)/"original.bin"
    target=Path(d)/"separate-test-copy.bin"
    source.write_bytes(original)
    command=[sys.executable,str(TOOLS/"mc02_cash_test_copy.py"),
             str(source),"--expected-cash","900","--cash","14400","--output",str(target)]
    subprocess.run(command,check=True,capture_output=True,text=True)
    assert not target.exists(),"dry run wrote data"
    subprocess.run(command+["--create-test-copy"],check=True,capture_output=True,text=True)
    assert validate(target.read_bytes())==14400
    assert source.read_bytes()==original
    duplicated=subprocess.run(command+["--create-test-copy"],capture_output=True,text=True)
    assert duplicated.returncode!=0,"overwrote existing test copy"
print("PASS: MC02 strict validation, all checksums, balance patch, dry run, no-overwrite and source protection")

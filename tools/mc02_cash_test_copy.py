#!/usr/bin/env python3
"""Experimental OFFLINE MC02 Cash patch on a *new test copy* only.

Requires a private test profile. NEVER use as an in-place save editor.
Only the 62688-byte Xbox 360 recompiled Most Wanted MC02 format is supported.
Two authentic saves confirmed the cash value field and stored hash algorithms;
game-side acceptance of patched saves has not yet been verified.
"""
import argparse
import hashlib
from pathlib import Path
import struct
from inspect_mc02 import inspect, ea_crc32

EXPECTED_LEN=62688
CASH_OFFSET=0x4039
CASH_MAX=999_999_999

def validate(data):
    if len(data)!=EXPECTED_LEN:
        raise ValueError(f"expected {EXPECTED_LEN}-byte MC02 save, got {len(data)}")
    analysis=inspect(data)
    if not all(x["valid"] for x in analysis["crc_ea"]) or not analysis["md5"]["valid"]:
        raise ValueError("save checksum failed; refusing to touch damaged or unknown input")
    return struct.unpack_from(">I",data,CASH_OFFSET)[0]

def build_test_copy(original,expected_cash,target_cash):
    if not (0<=expected_cash<=CASH_MAX and 0<=target_cash<=CASH_MAX):
        raise ValueError("cash must be between 0 and 999,999,999")
    current_cash=validate(original)
    if current_cash!=expected_cash:
        raise ValueError(f"expected current cash {expected_cash}, found {current_cash}")
    if current_cash==target_cash:
        raise ValueError("target cash already equals current cash")
    out=bytearray(original)
    struct.pack_into(">I",out,CASH_OFFSET,target_cash)
    # MC02 dependency order: MD5(body) -> EA CRC2(data incl. MD5)
    # -> EA CRC3(header incl. CRC2). CRC1(header metadata) also recomputed.
    struct.pack_into(">I",out,0x10,ea_crc32(out[0x1C:0x24]))
    out[-16:]=hashlib.md5(out[0x34:-16]).digest()
    struct.pack_into(">I",out,0x14,ea_crc32(out[0x24:]))
    struct.pack_into(">I",out,0x18,ea_crc32(out[:0x18]))
    if validate(bytes(out))!=target_cash:
        raise AssertionError("internal output validation failed")
    return bytes(out)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("source",type=Path)
    p.add_argument("--expected-cash",type=int,required=True,
                   help="exact known cash in unmodified source file")
    p.add_argument("--cash",type=int,required=True,help="target test cash")
    p.add_argument("--output",type=Path,help="new separate test output file (must not exist)")
    p.add_argument("--create-test-copy",action="store_true",
                   help="explicitly enable writing to --output (not source)")
    args=p.parse_args()
    source=args.source.resolve()
    if args.output is not None and source==args.output.resolve():
        p.error("In-place editing is forbidden")
    if args.create_test_copy and args.output is None:
        p.error("--output is required")
    try:
        before=source.read_bytes()
        patched=build_test_copy(before,args.expected_cash,args.cash)
        print(f"Input valid: {len(before)} bytes, Cash {args.expected_cash}")
        print(f"Test copy valid: cash {args.cash}; MD5 and 3 EA CRC32 recomputed")
        print("WARNING: Game-side load acceptance is NOT yet verified.")
        if not args.create_test_copy:
            print("DRY RUN: no files written; specify --create-test-copy and --output.")
            return
        output=args.output
        # Exclusive create: neither source nor pre-existing outputs are overwritten.
        try:
            with output.open("xb") as f:
                f.write(patched)
            readback=output.read_bytes()
            if readback!=patched or validate(readback)!=args.cash:
                raise RuntimeError("written copy verification failed")
        except Exception:
            if output.exists():
                try:
                    if output.read_bytes()!=before:
                        output.unlink()
                except OSError:
                    pass
            raise
        if hashlib.sha256(source.read_bytes()).digest()!=hashlib.sha256(before).digest():
            raise RuntimeError("source changed concurrently (unexpected); stop and inspect")
        print(f"Created test-only save: {output}")
    except (ValueError,OSError,RuntimeError) as e:
        p.error(str(e))
if __name__=="__main__":
    main()

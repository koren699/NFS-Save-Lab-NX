#!/usr/bin/env python3
"""Read-only MC02 profile validator and numeric candidate scanner.

No save bytes are changed. A matching byte pattern is NOT a verified game field.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

MAX_FILE = 8 * 1024 * 1024

def _crc_table():
    result=[]
    for i in range(256):
        v=i<<24
        for _ in range(8):
            v=((v<<1)^(0x04C11DB7 if v & 0x80000000 else 0)) & 0xffffffff
        result.append(v)
    return result

CRC_TABLE=_crc_table()

def ea_crc32(data):
    if len(data)<4:
        raise ValueError("Not enough data for EA CRC32")
    c=(~int.from_bytes(data[:4],"big"))&0xffffffff
    for b in data[4:]:
        c=(((c<<8)|b)^CRC_TABLE[c>>24])&0xffffffff
    return (~c)&0xffffffff

def find_offsets(blob,value):
    pattern={
      "u32_be":int(value).to_bytes(4,"big"),
      "u32_le":int(value).to_bytes(4,"little"),
      "float_be":struct.pack(">f",float(value)),
      "float_le":struct.pack("<f",float(value)),
    }
    out={}
    for label,pat in pattern.items():
        offsets=[]
        i=blob.find(pat)
        while i>=0:
            offsets.append(f"0x{i:04X}")
            i=blob.find(pat,i+1)
        out[label]=offsets
    return out

def inspect(data,cash=None,bounty=None):
    if len(data)<64 or len(data)>MAX_FILE:
        raise ValueError("Unsupported save size")
    if data[:4]!=b"MC02":
        raise ValueError("Not an MC02 save")
    size,unknown,data_len=struct.unpack_from(">III",data,4)
    if size!=len(data) or data_len!=len(data)-0x24:
        raise ValueError("MC02 length header does not match file length")
    saved_crc=struct.unpack_from(">III",data,0x10)
    calculated=(ea_crc32(data[0x1C:0x24]),ea_crc32(data[0x24:]),
                ea_crc32(data[:0x18]))
    stored_md5=data[-16:].hex()
    calculated_md5=hashlib.md5(data[0x34:-16]).hexdigest()
    out={
      "format":"MC02", "size":len(data),
      "crc_ea":[{"stored":f"{a:08x}","calculated":f"{b:08x}","valid":a==b}
                for a,b in zip(saved_crc,calculated)],
      "md5":{"stored":stored_md5,"calculated":calculated_md5,
             "valid":stored_md5==calculated_md5},
      "warning":"Patterns are NOT established Cash/Bounty offsets. Read-only."
    }
    if cash is not None:out["cash_candidates"]=find_offsets(data,cash)
    if bounty is not None:out["bounty_candidates"]=find_offsets(data,bounty)
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("save",type=Path)
    p.add_argument("--cash",type=int)
    p.add_argument("--bounty",type=int)
    a=p.parse_args()
    for value in (a.cash,a.bounty):
        if value is not None and not 0<=value<=0xffffffff:
            p.error("Values must be between 0 and 4294967295")
    print(json.dumps(inspect(a.save.read_bytes(),a.cash,a.bounty),indent=2))

if __name__=="__main__":
    main()

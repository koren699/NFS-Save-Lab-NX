#!/usr/bin/env python3
"""Replace libnx NRO hbmenu icon with a 256x256 JPEG without recompilation.
Keeps NRO executable, NACP settings and appended HD poster payload unchanged.
Example: python tools/replace_nro_icon.py base.nro icon.jpg final.nro
"""
from pathlib import Path
from io import BytesIO
from PIL import Image
import argparse,struct

def swap_icon(b,jpg):
    if Image.open(BytesIO(jpg)).size!=(256,256) or not jpg.startswith(b"\xff\xd8"):
        raise ValueError("hbmenu icon must be 256x256 JPEG")
    if b[16:20]!=b"NRO0":raise ValueError("Not a Switch NRO")
    nro_size=struct.unpack_from("<I",b,0x18)[0]
    if b[nro_size:nro_size+4]!=b"ASET":raise ValueError("ASET missing")
    (magic,version,icon_off,icon_len,nacp_off,nacp_len,
     romfs_off,romfs_len)=struct.unpack_from("<IIQQQQQQ",b,nro_size)
    if version or icon_off!=56 or nacp_len!=16384:
        raise ValueError("Unexpected asset layout")
    nacp=b[nro_size+nacp_off:nro_size+nacp_off+nacp_len]
    romfs=b[nro_size+romfs_off:nro_size+romfs_off+romfs_len] if romfs_len else b""
    if len(nacp)!=nacp_len or len(romfs)!=romfs_len:raise ValueError("Truncated")
    end=max(icon_off+icon_len,nacp_off+nacp_len,romfs_off+romfs_len)
    trailer=b[nro_size+end:]
    next_nacp=56+len(jpg)
    next_romfs=next_nacp+nacp_len if romfs_len else 0
    new_header=struct.pack("<IIQQQQQQ",magic,version,56,len(jpg),
                           next_nacp,nacp_len,next_romfs,romfs_len)
    result=b[:nro_size]+new_header+jpg+nacp+romfs+trailer
    assert result[:nro_size]==b[:nro_size] and result.endswith(trailer)
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("nro",type=Path)
    p.add_argument("jpeg",type=Path)
    p.add_argument("output",type=Path)
    a=p.parse_args()
    if a.output.exists():p.error("Output already exists; refusing overwrite")
    a.output.write_bytes(swap_icon(a.nro.read_bytes(),a.jpeg.read_bytes()))
    print("Created",a.output)

if __name__=="__main__":main()

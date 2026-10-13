#!/usr/bin/env python3
"""Bundle three raw RGB565 posters after an existing NRO's asset section.

This is v0.8's optional self-contained artwork extension. The NRO scanner
reads the last 20 bytes to find the appended data. If no trailer exists, it
uses embedded v0.7 fallback posters. This tool preserves the compiled NRO.
The three *.rgb565 files are 336*360*2 bytes, little-endian.
"""
import argparse
import struct
from pathlib import Path

MAGIC=b"NFSLABP8"
WIDTH,HEIGHT=336,360
EACH=WIDTH*HEIGHT*2

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("nro",type=Path)
    p.add_argument("mw_rgb565",type=Path)
    p.add_argument("u2_rgb565",type=Path)
    p.add_argument("cx_rgb565",type=Path)
    p.add_argument("output",type=Path)
    a=p.parse_args()
    original=a.nro.read_bytes()
    if len(original)<64 or original[16:20]!=b"NRO0":p.error("Expected NRO0 binary")
    expected=int.from_bytes(original[24:28],"little")
    if expected>=len(original) or original[expected:expected+4]!=b"ASET":p.error("Bad asset section")
    if original.endswith(MAGIC):p.error("Already bundled")
    if a.nro.resolve()==a.output.resolve():p.error("Output must differ from input")
    if a.output.exists():p.error("Output exists; refusing to overwrite")
    pictures=[path.read_bytes() for path in (a.mw_rgb565,a.u2_rgb565,a.cx_rgb565)]
    if any(len(pic)!=EACH for pic in pictures):p.error("Each poster must be 336x360 RGB565-LE")
    payload=b"".join(pictures)
    trailer=struct.pack("<8sIHHI",MAGIC,1,WIDTH,HEIGHT,len(payload))
    with a.output.open("xb") as f:f.write(original+payload+trailer)
    made=a.output.read_bytes()
    if made[:len(original)]!=original or made[-20:]!=trailer:
        p.error("Verification failed")
    print("Created",a.output,"bytes:",len(made))
if __name__=="__main__":main()

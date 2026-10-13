#!/usr/bin/env python3
"""Embed three indexed pixel-art game posters as static C arrays in the NRO.
Uses only Python's standard library. Run before compiling source/main.c.
"""
import base64
import json
from pathlib import Path
import zlib

ROOT = Path(__file__).resolve().parents[1]
JSON = ROOT / "assets" / "posters_embedded.json"
OUT = ROOT / "source" / "poster_embedded.h"

def c_bytes(name, values, wrap=18):
    data = list(values)
    lines=[f"static const unsigned char {name}[{len(data)}] = {{"]
    for i in range(0,len(data),wrap):
        lines.append("    " + ", ".join(str(x) for x in data[i:i+wrap]) + ",")
    lines.append("};")
    return "\n".join(lines)

def main():
    obj=json.loads(JSON.read_text(encoding="utf-8"))
    width,height,ncolors=obj["width"],obj["height"],obj["palette_colors"]
    if width!=80 or height!=100 or ncolors!=32: raise ValueError("Unexpected embedded poster format")
    declarations=["/* GENERATED automatically by tools/generate_poster_header.py. */",
                  "#ifndef NFSLAB_POSTER_EMBEDDED_H",
                  "#define NFSLAB_POSTER_EMBEDDED_H",
                  f"#define POSTER_W {width}",
                  f"#define POSTER_H {height}"]
    for name in ("mw","u2","cx"):
        binary=zlib.decompress(base64.b64decode(obj["images"][name],validate=True))
        pal=binary[:ncolors*3]
        inds=binary[ncolors*3:]
        if len(binary)!=ncolors*3+width*height or len(inds)!=width*height:
            raise ValueError(f"Corrupted {name} poster")
        if max(inds)>=ncolors:raise ValueError("Palette index out of range")
        declarations.append(c_bytes(f"poster_{name}_palette",pal))
        declarations.append(c_bytes(f"poster_{name}_indices",inds))
    declarations.extend([
      "static const unsigned char *const poster_palettes[3]={poster_mw_palette,poster_u2_palette,poster_cx_palette};",
      "static const unsigned char *const poster_indices[3]={poster_mw_indices,poster_u2_indices,poster_cx_indices};",
      "#endif",
    ])
    OUT.write_text("\n".join(declarations)+"\n",encoding="utf-8")
    print("Embedded pixel posters into generated header:",OUT.name,OUT.stat().st_size,"bytes")
if __name__=="__main__":
    main()

#!/bin/bash
set -e
cd "$(dirname "$0")"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITA64="$DEVKITPRO/devkitA64"
export PATH="$DEVKITA64/bin:$PATH"
make -j4
printf '\nBuilt: %s/nfs_save_lab_nx.nro\n' "$PWD"

#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from pathlib import Path

def out_paths(out_dir: Path, header: Path):
    stem = header.stem
    return (out_dir / f"{stem}.generated.h", out_dir / f"{stem}.refl.gen.cpp")

def mtime(p: Path) -> float:
    try:
        return p.stat().st_mtime
    except FileNotFoundError:
        return 0.0

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jreflectgen", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("headers", nargs="+")
    args = ap.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    exe = Path(args.jreflectgen)
    exe_m = mtime(exe)

    dirty = []
    for h in args.headers:
        hp = Path(h)
        out_h, out_cpp = out_paths(out_dir, hp)

        # Rebuild if outputs missing OR header newer OR generator exe newer
        if (not out_h.exists()) or (not out_cpp.exists()):
            dirty.append(str(hp))
            continue

        newest_out = max(mtime(out_h), mtime(out_cpp))
        if mtime(hp) > newest_out or exe_m > newest_out:
            dirty.append(str(hp))

    # If nothing changed, do nothing (and don't spam logs)
    if not dirty:
        # Still touch a stamp file if provided by env (optional)
        return 0

    # Call generator ONCE with only dirty headers (nice output!)
    cmd = [str(exe), "--out", str(out_dir), *dirty]
    return subprocess.call(cmd)

if __name__ == "__main__":
    sys.exit(main())

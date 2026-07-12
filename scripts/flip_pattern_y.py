#!/usr/bin/env python3
"""Flip normalized pattern coordinates vertically."""

import sys


for line in sys.stdin:
    fields = line.split()
    if not fields:
        continue
    if len(fields) != 2:
        raise SystemExit(f"expected two coordinates, got: {line.rstrip()}")

    x, y = map(float, fields)
    print(f"{x:g}\t{1.0 - y:g}")

#!/usr/bin/env python3
"""Print the pip stim wheel's PYBIND11_BUILD_ABI for cmake/python.cmake."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys


def main() -> int:
    try:
        import stim
    except ImportError:
        print("error: stim is not installed", file=sys.stderr)
        return 1

    sos = sorted(pathlib.Path(stim.__file__).parent.glob("_stim*.so"))
    if not sos:
        print(f"error: no _stim*.so under {pathlib.Path(stim.__file__).parent}", file=sys.stderr)
        return 1

    text = subprocess.check_output(["strings", sos[0]], text=True, errors="ignore")
    ids = sorted(set(re.findall(r"__pybind11_internals\S+__", text)))
    if not ids:
        print(f"error: no pybind11 internals ID in {sos[0].name}", file=sys.stderr)
        return 1

    match = re.search(r"(_cxxabi\d+)", ids[0])
    if not match:
        print(f"error: no _cxxabi* fragment in {ids[0]}", file=sys.stderr)
        return 1

    print(f'PYBIND11_BUILD_ABI="{match.group(1)}"')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Check formatting of Nix files in the GroveDB tree."""

import glob
import os
import shutil
import subprocess
import sys


def _find_grovedb_root():
    """Find the GroveDB root (directory containing flake.nix)."""
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isfile(os.path.join(d, "flake.nix")):
            return d
        d = os.path.dirname(d)
    return None


def _check_formatter():
    """Check if nixfmt is available."""
    if shutil.which("nixfmt"):
        return "nixfmt"

    print('Error: "nixfmt" is not available.')
    print("Run from inside the devenv (nix develop) or install nixfmt.")
    sys.exit(1)


def _get_nix_files(root):
    """Get .nix files under root using glob."""
    pattern = os.path.join(root, "**", "*.nix")
    files = sorted(glob.glob(pattern, recursive=True))
    return [os.path.relpath(f, root) for f in files]


def main():
    root = _find_grovedb_root()
    if root is None:
        sys.exit("Error: cannot find GroveDB root (no flake.nix found)")

    _check_formatter()
    files = _get_nix_files(root)

    if not files:
        print("No Nix files found")
        sys.exit(0)

    print(f"Checking {len(files)} Nix file(s)...")

    check_cmd = ["nixfmt", "--check"] + files
    fix_cmd = f'nixfmt {" ".join(files)}'

    try:
        subprocess.check_call(check_cmd, cwd=root)
    except subprocess.CalledProcessError:
        print()
        print("Nix formatting issues detected.")
        print(f"To fix, run: cd {root} && {fix_cmd}")
        sys.exit(1)

    print("OK")


if __name__ == "__main__":
    main()

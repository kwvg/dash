#!/usr/bin/env python3
"""Build Doxygen docs and gcovr coverage reports into ./output/."""

import glob
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
OUTPUT = os.path.join(ROOT, "output")
DOXYGEN_DIR = os.path.join(ROOT, "contrib", "doxygen")
GCOVR_DIR = os.path.join(ROOT, "contrib", "gcovr")
DOXYFILE = os.path.join(DOXYGEN_DIR, "Doxyfile")


def git_hash():
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=ROOT, stderr=subprocess.DEVNULL,
        ).decode().strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unknown"


def inject_git_hash(directory, hash_str):
    for path in glob.glob(os.path.join(directory, "*.html")):
        with open(path, "r") as f:
            content = f.read()
        content = content.replace("build unknown", f"build {hash_str}")
        with open(path, "w") as f:
            f.write(content)


def build_doxygen(hash_str):
    print("==> Building Doxygen documentation...")

    # Temporarily set PROJECT_NUMBER to the git hash
    with open(DOXYFILE, "r") as f:
        original = f.read()
    patched = re.sub(
        r"^(PROJECT_NUMBER\s*=\s*).*$",
        rf"\g<1>{hash_str}",
        original,
        flags=re.MULTILINE,
    )
    try:
        with open(DOXYFILE, "w") as f:
            f.write(patched)
        subprocess.check_call(["doxygen", "Doxyfile"], cwd=DOXYGEN_DIR)
    finally:
        # Always restore the Doxyfile back to unknown
        with open(DOXYFILE, "w") as f:
            f.write(original)

    src = os.path.join(DOXYGEN_DIR, "output", "html")
    if not os.path.isdir(src):
        raise RuntimeError(f"Doxygen output not found at {src}")

    # Copy HTML contents into ./output/ (the root serves the API docs)
    if os.path.isdir(OUTPUT):
        for item in os.listdir(src):
            s = os.path.join(src, item)
            d = os.path.join(OUTPUT, item)
            if os.path.isdir(s):
                if os.path.isdir(d):
                    shutil.rmtree(d)
                shutil.copytree(s, d)
            else:
                shutil.copy2(s, d)
    else:
        shutil.copytree(src, OUTPUT)

    print("    Doxygen output -> output/")


def build_gcovr(name, config, hash_str):
    print(f"==> Building {name} coverage report...")
    os.makedirs(os.path.join(GCOVR_DIR, "output", name), exist_ok=True)
    subprocess.check_call(["gcovr", "--config", config], cwd=GCOVR_DIR)

    src = os.path.join(GCOVR_DIR, "output", name)
    if not os.path.isdir(src):
        raise RuntimeError(f"gcovr {name} output not found at {src}")

    inject_git_hash(src, hash_str)

    dst = os.path.join(OUTPUT, name)
    os.makedirs(OUTPUT, exist_ok=True)
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)

    print(f"    {name} coverage -> output/{name}/")


def main():
    hash_str = git_hash()

    # Doxygen is mandatory
    build_doxygen(hash_str)

    # Coverage reports are optional (skip on failure)
    coverage_steps = [
        ("test coverage", lambda: build_gcovr("test", "test.cfg", hash_str)),
        ("fuzz coverage", lambda: build_gcovr("fuzz", "fuzz.cfg", hash_str)),
    ]

    results = [("Doxygen", True)]
    for label, fn in coverage_steps:
        try:
            fn()
            results.append((label, True))
        except Exception as e:
            print(f"    SKIPPED ({e})", file=sys.stderr)
            results.append((label, False))

    print()
    print("Summary:")
    for label, ok in results:
        status = "OK" if ok else "SKIPPED"
        print(f"  {label:20s} {status}")


if __name__ == "__main__":
    main()

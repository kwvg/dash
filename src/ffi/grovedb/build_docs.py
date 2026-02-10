#!/usr/bin/env python3
"""Build Doxygen docs and gcovr coverage reports into ./output/."""

import base64
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
GUIDE_SRC = os.path.join(ROOT, "contrib", "guide")
GUIDE_DST = os.path.join(DOXYGEN_DIR, "_guide")
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


def preprocess_guide():
    """Copy contrib/guide/ into contrib/doxygen/_guide/ with transforms."""
    print("==> Preprocessing guide markdown...")

    if os.path.isdir(GUIDE_DST):
        shutil.rmtree(GUIDE_DST)
    os.makedirs(GUIDE_DST, exist_ok=True)

    # Copy assets directory
    src_assets = os.path.join(GUIDE_SRC, "assets")
    dst_assets = os.path.join(GUIDE_DST, "assets")
    if os.path.isdir(src_assets):
        shutil.copytree(src_assets, dst_assets)

    # Process each markdown file
    for md in sorted(glob.glob(os.path.join(GUIDE_SRC, "*.md"))):
        name = os.path.basename(md)
        # Skip README.md — its content is already on the mainpage
        if name == "README.md":
            continue
        with open(md, "r") as f:
            content = f.read()

        # --- Transform A: Page ID injection ---
        stem = os.path.splitext(name)[0]
        stem = re.sub(r"^\d+-", "", stem).lower()
        page_id = f"guide-{stem}"

        content = re.sub(
            r"^(# .+)",
            rf"\1 {{#{page_id}}}",
            content,
            count=1,
            flags=re.MULTILINE,
        )

        # --- Transform B: Mermaid fenced blocks → HTML divs ---
        def mermaid_to_html(m):
            source = m.group(1)
            encoded = base64.b64encode(source.encode()).decode()
            return (
                "@htmlonly\n"
                f'<div class="mermaid" data-mermaid-source="{encoded}">\n'
                "Loading diagram...\n"
                "</div>\n"
                "@endhtmlonly"
            )

        content = re.sub(
            r"```mermaid\n(.*?)```",
            mermaid_to_html,
            content,
            flags=re.DOTALL,
        )

        # --- Transform C: Example source links ---
        content = re.sub(
            r"\]\(\.\.\/libgrovedb\/contrib\/examples\/(\w+\.cpp)\)",
            r"](@ref \1)",
            content,
        )

        with open(os.path.join(GUIDE_DST, name), "w") as f:
            f.write(content)

    print(f"    Preprocessed guide -> {os.path.relpath(GUIDE_DST, ROOT)}")


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


def patch_navtree():
    """Inject coverage report links into the Doxygen sidebar nav tree."""
    navtree_js = os.path.join(OUTPUT, "navtreedata.js")
    if not os.path.isfile(navtree_js):
        return
    with open(navtree_js, "r") as f:
        content = f.read()

    # Insert as top-level siblings of "GroveDB for C++" (same depth in NAVTREE[])
    # Each entry: [ "Label", "^url", null ]  — the ^ prefix means absolute URL
    # \u2009 = thin space before the arrow for padding
    coverage_entries = (
        ',\n'
        '  [ "Test Coverage \\u2009\\u2197", "^test/index.html", null ],\n'
        '  [ "Fuzz Coverage \\u2009\\u2197", "^fuzz/index.html", null ]'
    )
    # Close of NAVTREE: the root entry ends with ] ]\n];
    # We insert after the root's closing ] ] but before the outer ];
    content = content.replace(
        "  ] ]\n];",
        "  ] ]" + coverage_entries + "\n];",
    )
    with open(navtree_js, "w") as f:
        f.write(content)


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

    # Preprocess guide markdown into staging directory
    preprocess_guide()

    # Doxygen is mandatory
    build_doxygen(hash_str)

    # Inject coverage links into sidebar nav tree
    patch_navtree()

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

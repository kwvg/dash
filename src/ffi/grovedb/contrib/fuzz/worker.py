#!/usr/bin/env python3
"""Standalone AFL++ worker for a single GroveDB fuzz harness on macOS ARM64.

Runs a single AFL++ instance against a given harness binary, writing to a
scratch directory that must be backed by a RAM disk.  Designed to be used
standalone or composed by campaign.py.
"""

import argparse
import os
import platform
import shutil
import signal
import subprocess
import sys


def _check_platform():
    if sys.platform != "darwin":
        sys.exit("Error: this tool only supports macOS")
    if platform.machine() != "arm64":
        sys.exit("Error: this tool only supports ARM64 (Apple Silicon) Macs")


def _resolve_tool(name):
    path = shutil.which(name)
    if path is None:
        sys.exit(f"Error: required tool '{name}' not found in PATH")
    return path


def _find_mount_point(path):
    """Walk up from path to the nearest mount point."""
    path = os.path.realpath(path)
    while not os.path.exists(path):
        path = os.path.dirname(path)
    while not os.path.ismount(path):
        path = os.path.dirname(path)
    return path


def _is_ramdisk(path):
    """Check whether path is backed by a macOS RAM disk (disk image)."""
    mount_point = _find_mount_point(path)
    result = subprocess.run(
        ["diskutil", "info", mount_point],
        capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        return False
    for line in result.stdout.splitlines():
        if "Protocol:" in line and "Disk Image" in line:
            return True
    return False


def _setup_dirs(scratch, corpus):
    """Create input/output dirs and seed the corpus."""
    input_dir = os.path.join(scratch, "input")
    output_dir = os.path.join(scratch, "output")
    os.makedirs(input_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)

    if corpus and os.path.isdir(corpus):
        for name in os.listdir(corpus):
            src = os.path.join(corpus, name)
            if os.path.isfile(src):
                shutil.copy2(src, input_dir)

    if not os.listdir(input_dir):
        with open(os.path.join(input_dir, "seed"), "w") as f:
            f.write("A\n")

    return input_dir, output_dir


def main():
    _check_platform()

    parser = argparse.ArgumentParser(
        description="Standalone AFL++ worker for a single fuzz harness"
    )
    parser.add_argument("--harness", required=True,
                        help="Path to the AFL++-instrumented fuzz harness binary")
    parser.add_argument("--scratch", required=True,
                        help="Scratch directory (must be on a RAM disk)")
    parser.add_argument("--corpus",
                        help="Seed corpus directory for this target")
    parser.add_argument("--timeout", type=int, default=5000,
                        help="Per-execution timeout in milliseconds (default: 5000)")

    args = parser.parse_args()

    if not os.path.isfile(args.harness) or not os.access(args.harness, os.X_OK):
        sys.exit(f"Error: harness not found or not executable: {args.harness}")

    if not _is_ramdisk(args.scratch):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        ramdisk_py = os.path.join(script_dir, "ramdisk.py")
        print(
            f"Error: scratch directory is not on a RAM disk: {args.scratch}\n"
            f"\n"
            f"AFL++ writes heavily to disk.  Use a RAM disk to avoid SSD wear.\n"
            f"Create one with:\n"
            f"\n"
            f"  python3 {ramdisk_py} create --size 2048 --name my_fuzz\n"
            f"\n"
            f"Then pass /Volumes/my_fuzz as --scratch.",
            file=sys.stderr,
        )
        sys.exit(1)

    afl_fuzz = _resolve_tool("afl-fuzz")
    input_dir, output_dir = _setup_dirs(args.scratch, args.corpus)

    env = os.environ.copy()
    env["AFL_SKIP_CPUFREQ"] = "1"
    env["AFL_NO_AFFINITY"] = "1"
    env["AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES"] = "1"
    env.setdefault("AFL_MAP_SIZE", "65536")
    env["AFL_TMPDIR"] = args.scratch
    env["AFL_NO_UI"] = "1"

    cmd = [
        afl_fuzz,
        "-i", input_dir,
        "-o", output_dir,
        "-t", str(args.timeout),
        "--", args.harness,
    ]

    afl_proc = subprocess.Popen(cmd, env=env, stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)

    def forward_signal(signum, frame):
        afl_proc.terminate()

    signal.signal(signal.SIGTERM, forward_signal)
    signal.signal(signal.SIGINT, forward_signal)

    afl_proc.wait()
    sys.exit(afl_proc.returncode)


if __name__ == "__main__":
    main()

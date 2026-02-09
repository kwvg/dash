#!/usr/bin/env python3
"""RAM disk create/destroy utility for macOS ARM64."""

import argparse
import platform
import shutil
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


def cmd_create(args):
    _check_platform()
    hdiutil = _resolve_tool("hdiutil")
    diskutil = _resolve_tool("diskutil")

    sectors = args.size * 2048
    result = subprocess.run(
        [hdiutil, "attach", "-nomount", f"ram://{sectors}"],
        capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        sys.exit(f"Error: hdiutil attach failed: {result.stderr.strip()}")

    device = result.stdout.strip()
    result = subprocess.run(
        [diskutil, "erasevolume", "HFS+", args.name, device],
        capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        # Try to detach the device on failure
        subprocess.run([hdiutil, "detach", device], capture_output=True, check=False)
        sys.exit(f"Error: diskutil erasevolume failed: {result.stderr.strip()}")

    mount_path = f"/Volumes/{args.name}"
    print(mount_path)


def cmd_destroy(args):
    _check_platform()
    diskutil = _resolve_tool("diskutil")

    mount_path = f"/Volumes/{args.name}"
    result = subprocess.run(
        [diskutil, "eject", mount_path],
        capture_output=True, text=True, check=False,
    )
    if result.returncode != 0:
        sys.exit(f"Error: diskutil eject failed: {result.stderr.strip()}")

    print(f"Destroyed {mount_path}")


def main():
    parser = argparse.ArgumentParser(description="macOS RAM disk manager")
    sub = parser.add_subparsers(dest="command", required=True)

    p_create = sub.add_parser("create", help="Create a RAM disk")
    p_create.add_argument("--size", type=int, required=True, help="Size in MB")
    p_create.add_argument("--name", required=True, help="Volume name")

    p_destroy = sub.add_parser("destroy", help="Destroy a RAM disk")
    p_destroy.add_argument("--name", required=True, help="Volume name")

    args = parser.parse_args()
    if args.command == "create":
        cmd_create(args)
    elif args.command == "destroy":
        cmd_destroy(args)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Dash Core Nix Environment Setup

This script configures the build environment with the correct dynamic linker
and compiler flags, following the Guix approach (contrib/guix/libexec/build.sh).

Unlike the old patchelf approach, this sets LDFLAGS during configure so binaries
are built with the correct interpreter from the start.

Inspired by contrib/guix/libexec/build.sh lines 148-163 and 237.
"""

import sys
from typing import Optional


def get_dynamic_linker(host: str) -> Optional[str]:
  """
  Return the correct dynamic linker path for HOST.

  Based on contrib/guix/libexec/build.sh lines 148-163.

  Args:
    host: Target triplet (e.g., 'x86_64-pc-linux-gnu')

  Returns:
    Path to dynamic linker, or None if not applicable
  """
  # Normalize host format (handle both pc-linux-gnu and linux-gnu variants)
  host_normalized = host.replace('pc-', '')

  linkers = {
    'x86_64-linux-gnu': '/lib64/ld-linux-x86-64.so.2',
    'arm-linux-gnueabihf': '/lib/ld-linux-armhf.so.3',
    'aarch64-linux-gnu': '/lib/ld-linux-aarch64.so.1',
    'riscv64-linux-gnu': '/lib/ld-linux-riscv64-lp64d.so.1',
    'powerpc64-linux-gnu': '/lib64/ld64.so.1',
    'powerpc64le-linux-gnu': '/lib64/ld64.so.2',
  }

  return linkers.get(host_normalized)


def setup_ldflags(host: str, extra_flags: str = "") -> str:
  """
  Configure LDFLAGS with dynamic linker.

  Based on contrib/guix/libexec/build.sh line 237:
    HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$glibc_dynamic_linker -static-libstdc++ -Wl,-O2"

  Args:
    host: Target triplet
    extra_flags: Additional LDFLAGS to append

  Returns:
    Complete LDFLAGS string
  """
  linker = get_dynamic_linker(host)

  # Base flags (always applied for Linux targets)
  base_flags = "-Wl,--as-needed"

  if linker and 'linux' in host:
    # Full Guix-style LDFLAGS for Linux
    flags = f"{base_flags} -Wl,--dynamic-linker={linker} -static-libstdc++ -Wl,-O2"
  else:
    # Non-Linux targets (macOS, Windows, etc.)
    flags = base_flags

  if extra_flags:
    flags = f"{flags} {extra_flags}"

  return flags


def setup_environment(host: str, print_exports: bool = True) -> dict:
  """
  Set up complete build environment for HOST.

  Args:
    host: Target triplet
    print_exports: If True, print shell export commands

  Returns:
    Dictionary of environment variables
  """
  env = {}

  # LDFLAGS with dynamic linker
  ldflags = setup_ldflags(host)
  env['LDFLAGS'] = ldflags

  # Additional environment setup
  env['HOST'] = host

  if print_exports:
    # Print shell export commands (for eval in shell)
    for key, value in env.items():
      print(f'export {key}="{value}"')

  return env


def main():
  """Main entry point for command-line usage."""
  if len(sys.argv) < 2:
    print("Usage: env_setup.py <HOST>", file=sys.stderr)
    print("", file=sys.stderr)
    print("Examples:", file=sys.stderr)
    print("  env_setup.py x86_64-pc-linux-gnu", file=sys.stderr)
    print("  env_setup.py arm-linux-gnueabihf", file=sys.stderr)
    print("  env_setup.py aarch64-linux-gnu", file=sys.stderr)
    print("", file=sys.stderr)
    print("Output: Shell export commands (use with eval)", file=sys.stderr)
    sys.exit(1)

  host = sys.argv[1]

  # Print environment setup commands
  setup_environment(host, print_exports=True)


if __name__ == '__main__':
  main()

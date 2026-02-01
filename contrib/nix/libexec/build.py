#!/usr/bin/env python3
"""
Dash Core Nix Build Script

Unlike the old approach which patched binaries after build,
this sets proper LDFLAGS during configure so binaries are
built correctly from the start.

Inspired by contrib/guix/libexec/build.sh but in Python.

Usage:
  build.py <target> [options]

Examples:
  build.py linux64_nowallet
  build.py linux64 --jobs 8
  build.py mac --skip-depends
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import Optional

# Import our environment setup module
import env_setup


# Build target configurations
# Based on ci/test/00_setup_env_*.sh files
TARGETS = {
  'linux64_nowallet': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': 'NO_WALLET=1 NO_QT=1',
    'config_opts': '--enable-reduce-exports --with-boost-process',
  },
  'linux64': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': '',
    'config_opts': '--enable-reduce-exports --with-boost-process --enable-qt',
  },
  'linux64_fuzz': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': 'NO_QT=1',
    'config_opts': '--enable-fuzz --with-sanitizers=fuzzer,address,undefined,integer',
  },
  'linux64_tsan': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': 'NO_QT=1',
    'config_opts': '--enable-suppress-external-warnings --with-sanitizers=thread',
  },
  'linux64_ubsan': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': '',
    'config_opts': '--enable-suppress-external-warnings --with-sanitizers=undefined',
  },
  'linux64_sqlite': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': 'NO_BDB=1',
    'config_opts': '--enable-reduce-exports --with-boost-process --enable-qt',
  },
  'linux64_multiprocess': {
    'host': 'x86_64-pc-linux-gnu',
    'dep_opts': 'MULTIPROCESS=1',
    'config_opts': '--enable-multiprocess --with-boost-process',
  },
  'arm-linux': {
    'host': 'arm-linux-gnueabihf',
    'dep_opts': '',
    'config_opts': '--enable-reduce-exports --enable-glibc-back-compat',
  },
  'mac': {
    'host': 'x86_64-apple-darwin',
    'dep_opts': '',
    'config_opts': '--enable-reduce-exports --enable-werror',
    'skip_tests': True,  # Cross-compile only
  },
  'win64': {
    'host': 'x86_64-w64-mingw32',
    'dep_opts': '',
    'config_opts': '--enable-reduce-exports',
  },
}


def run_command(cmd: list, env: Optional[dict] = None, cwd: Optional[Path] = None):
  """
  Run a command and handle errors.

  Args:
    cmd: Command to run as list
    env: Environment variables (inherits from os.environ if None)
    cwd: Working directory
  """
  if env is None:
    env = os.environ.copy()

  print(f"Running: {' '.join(str(c) for c in cmd)}")
  result = subprocess.run(cmd, env=env, cwd=cwd)

  if result.returncode != 0:
    print(f"Error: Command failed with exit code {result.returncode}", file=sys.stderr)
    sys.exit(result.returncode)


def build_depends(host: str, jobs: int, dep_opts: str, skip: bool = False):
  """
  Build depends/ for HOST.

  Args:
    host: Target triplet
    jobs: Number of parallel jobs
    dep_opts: Additional options for depends build
    skip: Skip depends build
  """
  if skip:
    print("Skipping depends build (--skip-depends)")
    return

  print(f"\n=== Building depends for {host} ===\n")

  # Build make command
  cmd = ['make', '-C', 'depends', f'HOST={host}', f'-j{jobs}']

  # Add dependency options
  if dep_opts:
    for opt in dep_opts.split():
      cmd.append(opt)

  run_command(cmd)


def configure_dashcore(host: str, config_opts: str, skip_depends: bool = False):
  """
  Run ./configure with proper flags.

  Args:
    host: Target triplet
    config_opts: Additional configure options
    skip_depends: Whether depends was skipped
  """
  print(f"\n=== Configuring Dash Core for {host} ===\n")

  # Set up environment with dynamic linker
  env = os.environ.copy()
  env_vars = env_setup.setup_environment(host, print_exports=False)
  env.update(env_vars)

  # Determine prefix path
  if skip_depends:
    prefix = Path.cwd()
  else:
    # Use depends prefix
    prefix = Path.cwd() / 'depends' / host

  # Build configure command
  args = [
    './configure',
    f'--prefix={prefix}',
  ]

  # Add target-specific options
  if config_opts:
    args.extend(config_opts.split())

  print(f"LDFLAGS: {env.get('LDFLAGS', '(not set)')}")
  run_command(args, env=env)


def build_dashcore(jobs: int):
  """
  Run make to build Dash Core.

  Args:
    jobs: Number of parallel jobs
  """
  print(f"\n=== Building Dash Core ===\n")

  cmd = ['make', f'-j{jobs}']
  run_command(cmd)


def verify_build(host: str):
  """
  Verify the build completed successfully.

  For Linux targets, check that binaries have the correct dynamic linker.

  Args:
    host: Target triplet
  """
  print(f"\n=== Verifying build ===\n")

  # Check if dashd was built
  dashd_path = Path('src/dashd')
  if not dashd_path.exists():
    print("Warning: src/dashd not found (might be cross-compile or nowallet build)")
    return

  # For Linux targets, verify dynamic linker
  if 'linux' in host:
    expected_linker = env_setup.get_dynamic_linker(host)
    if expected_linker:
      print(f"Checking dynamic linker for {dashd_path}...")
      try:
        result = subprocess.run(
          ['readelf', '-l', str(dashd_path)],
          capture_output=True,
          text=True,
        )
        if expected_linker in result.stdout:
          print(f"✓ Binary has correct interpreter: {expected_linker}")
        else:
          print(f"⚠ Warning: Expected interpreter {expected_linker} not found")
          print("Output:")
          print(result.stdout)
      except FileNotFoundError:
        print("Note: readelf not available, skipping verification")


def main():
  """Main entry point."""
  parser = argparse.ArgumentParser(
    description='Build Dash Core using Nix environment'
  )
  parser.add_argument(
    'target',
    choices=list(TARGETS.keys()),
    help='Build target'
  )
  parser.add_argument(
    '--jobs', '-j',
    type=int,
    default=os.cpu_count() or 1,
    help='Number of parallel jobs (default: cpu count)'
  )
  parser.add_argument(
    '--skip-depends',
    action='store_true',
    help='Skip depends build (use for native builds)'
  )
  parser.add_argument(
    '--dry-run',
    action='store_true',
    help='Print configuration and exit'
  )

  args = parser.parse_args()

  # Get target configuration
  target_config = TARGETS[args.target]
  host = target_config['host']
  dep_opts = target_config['dep_opts']
  config_opts = target_config['config_opts']

  print(f"=== Dash Core Build Configuration ===")
  print(f"Target: {args.target}")
  print(f"Host: {host}")
  print(f"Depends options: {dep_opts or '(none)'}")
  print(f"Configure options: {config_opts or '(none)'}")
  print(f"Jobs: {args.jobs}")
  print(f"Skip depends: {args.skip_depends}")
  print()

  if args.dry_run:
    print("Dry run - exiting")
    return

  # Build pipeline
  build_depends(host, args.jobs, dep_opts, skip=args.skip_depends)
  configure_dashcore(host, config_opts, skip_depends=args.skip_depends)
  build_dashcore(args.jobs)
  verify_build(host)

  print(f"\n✓ Build complete for {args.target}")


if __name__ == '__main__':
  main()

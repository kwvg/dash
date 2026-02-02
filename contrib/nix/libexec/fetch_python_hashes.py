#!/usr/bin/env python3
"""
Fetch SHA256 hashes for pinned Python packages.

This script fetches the SHA256 hashes for all pinned Python packages
from PyPI and converts them to Nix SRI format.

Usage:
  python3 fetch_python_hashes.py
"""

import hashlib
import subprocess
import sys
from typing import Dict


# Pinned versions from ci-slim.Dockerfile
PINNED_PACKAGES = {
  'codespell': '2.1.0',
  'flake8': '4.0.1',
  'lief': '0.13.2',
  'mypy': '0.981',
  'pyzmq': '24.0.1',
  'vulture': '2.6',
}

# GitHub packages
GITHUB_PACKAGES = {
  'dash_hash': {
    'owner': 'dashpay',
    'repo': 'dash_hash',
    'rev': 'v1.4.0',
  }
}


def fetch_pypi_hash(package: str, version: str) -> str:
  """
  Fetch SHA256 hash for a PyPI package.

  Args:
    package: Package name
    version: Package version

  Returns:
    SHA256 hash in SRI format (sha256-...)
  """
  url = f"https://files.pythonhosted.org/packages/source/{package[0]}/{package}/{package}-{version}.tar.gz"

  # Use nix-prefetch-url to fetch and get hash
  result = subprocess.run(
    ['nix-prefetch-url', url],
    capture_output=True,
    text=True,
  )

  if result.returncode != 0:
    print(f"Error fetching {package}: {result.stderr}", file=sys.stderr)
    return None

  # Get the base32 hash (first line of output)
  base32_hash = result.stdout.strip().split('\n')[0]

  # Convert to SRI format
  result = subprocess.run(
    ['nix', 'hash', 'convert', '--to', 'sri', '--hash-algo', 'sha256', base32_hash],
    capture_output=True,
    text=True,
  )

  if result.returncode != 0:
    print(f"Error converting hash for {package}: {result.stderr}", file=sys.stderr)
    return None

  return result.stdout.strip()


def fetch_github_hash(owner: str, repo: str, rev: str) -> str:
  """
  Fetch SHA256 hash for a GitHub repository.

  Args:
    owner: GitHub owner
    repo: Repository name
    rev: Git revision (tag or commit)

  Returns:
    SHA256 hash in SRI format
  """
  result = subprocess.run(
    ['nix-prefetch-github', owner, repo, '--rev', rev],
    capture_output=True,
    text=True,
  )

  if result.returncode != 0:
    print(f"Error fetching {owner}/{repo}: {result.stderr}", file=sys.stderr)
    # Try alternative method
    url = f"https://github.com/{owner}/{repo}/archive/{rev}.tar.gz"
    result = subprocess.run(
      ['nix-prefetch-url', '--unpack', url],
      capture_output=True,
      text=True,
    )

    if result.returncode != 0:
      return None

    base32_hash = result.stdout.strip().split('\n')[0]
    result = subprocess.run(
      ['nix', 'hash', 'convert', '--to', 'sri', '--hash-algo', 'sha256', base32_hash],
      capture_output=True,
      text=True,
    )
    return result.stdout.strip()

  # nix-prefetch-github returns JSON with sha256
  import json
  data = json.loads(result.stdout)
  return f"sha256-{data['sha256']}"


def main():
  """Fetch all hashes and print them."""
  print("# Fetching hashes for pinned Python packages\n")

  print("## PyPI packages:")
  for package, version in PINNED_PACKAGES.items():
    print(f"\n{package} = {version}")
    hash_val = fetch_pypi_hash(package, version)
    if hash_val:
      print(f"  hash = \"{hash_val}\";")
    else:
      print(f"  ERROR: Could not fetch hash")

  print("\n## GitHub packages:")
  for package, info in GITHUB_PACKAGES.items():
    print(f"\n{package} = {info['rev']}")
    hash_val = fetch_github_hash(info['owner'], info['repo'], info['rev'])
    if hash_val:
      print(f"  hash = \"{hash_val}\";")
    else:
      print(f"  ERROR: Could not fetch hash")


if __name__ == '__main__':
  main()

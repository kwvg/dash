#!/usr/bin/env python3
#
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check for formatting issues in Nix files using nixfmt.
"""

import subprocess
import sys


def check_nixfmt_install():
    """Check if nixfmt is installed."""
    try:
        subprocess.run(['nixfmt', '--version'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    except FileNotFoundError:
        print('Skipping Nix linting since nixfmt is not installed.')
        sys.exit(0)
    except subprocess.CalledProcessError:
        # Some versions of nixfmt don't support --version, try --help
        try:
            subprocess.run(['nixfmt', '--help'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        except (FileNotFoundError, subprocess.CalledProcessError):
            print('Skipping Nix linting since nixfmt is not installed.')
            sys.exit(0)


def get_nix_files():
    """Get list of Nix files to check."""
    try:
        files_cmd = [
            'git',
            'ls-files',
            '--',
            '*.nix',
        ]
        output = subprocess.run(files_cmd, stdout=subprocess.PIPE, universal_newlines=True, check=True)
        files = output.stdout.strip().split('\n')

        # Filter to only contrib/nix directory
        files = [f for f in files if f.startswith('contrib/nix/') and f]

        return files
    except subprocess.CalledProcessError:
        print('Error: git command failed')
        sys.exit(1)


def main():
    check_nixfmt_install()

    files = get_nix_files()

    if not files:
        print('No Nix files found in contrib/nix/')
        sys.exit(0)

    # Run nixfmt in check mode
    nixfmt_cmd = ['nixfmt', '--check'] + files

    try:
        subprocess.check_call(nixfmt_cmd)
    except subprocess.CalledProcessError:
        print()
        print('Nix formatting issues detected.')
        print('To fix, run: nixfmt ' + ' '.join(files))
        sys.exit(1)


if __name__ == '__main__':
    main()

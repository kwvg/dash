#!/usr/bin/env python3
#
# Copyright (c) 2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check for formatting issues in Nix files using nix fmt or nixfmt.
"""

import subprocess
import sys
import os


def check_formatter():
    """Check if nix fmt or nixfmt is available and return the command to use."""
    shell_prefix = ['bash', '-c', 'source ~/.zshrc 2>/dev/null || source ~/.bashrc 2>/dev/null || true; ']

    # First, try 'nix fmt'
    try:
        cmd = shell_prefix + ['nix fmt -- --version 2>/dev/null || nix fmt -- --help 2>/dev/null']
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        if result.returncode == 0:
            return ('nix-fmt', shell_prefix)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    # Then try 'nixfmt'
    try:
        cmd = shell_prefix + ['nixfmt --version 2>/dev/null || nixfmt --help 2>/dev/null']
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        if result.returncode == 0:
            return ('nixfmt', shell_prefix)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    print('Skipping Nix linting since neither "nix fmt" nor "nixfmt" is available.')
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
    formatter, shell_prefix = check_formatter()

    files = get_nix_files()

    if not files:
        print('No Nix files found in contrib/nix/')
        sys.exit(0)

    # Build the check command based on which formatter we're using
    if formatter == 'nix-fmt':
        # nix fmt expects files as arguments and uses -- to separate formatter args
        check_cmd = f'nix fmt -- --check {" ".join(files)}'
        fix_cmd = f'nix fmt'
    else:  # nixfmt
        check_cmd = f'nixfmt --check {" ".join(files)}'
        fix_cmd = f'nixfmt {" ".join(files)}'

    # Run formatter in check mode
    full_cmd = shell_prefix + [check_cmd]

    try:
        subprocess.check_call(full_cmd)
    except subprocess.CalledProcessError:
        print()
        print('Nix formatting issues detected.')
        print(f'To fix, run: {fix_cmd}')
        sys.exit(1)


if __name__ == '__main__':
    main()

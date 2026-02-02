# SPDX-FileCopyrightText: 2026 The Dash Core developers
# SPDX-License-Identifier: MIT

# Crate for GroveDB FFI using CXX
# Hybrid approach: Cargo for grovedb dependencies, native Meson for integration

project('grovedb_cxx',
  'rust',
  version: '4.0.0',
  license: 'MIT',
  meson_version: '>=1.9.0',
  default_options: [
    'warning_level=3',
    'rust_std=2018',
  ]
)

if meson.is_subproject()
  message('Including grovedb_cxx in librustdeps')
endif

# Get Rust module for CXX bridge generation
rust = import('rust')

# Build options
cargo_profile = get_option('cargo_profile')

# Find required tools
cargo = find_program('cargo', required: true)
cxxbridge = find_program('cxxbridge', required: false)

# Rust source files (for dependency tracking)
rust_sources = files(
  'src/lib.rs',
  'src/auxiliary.rs',
  'src/batch.rs',
  'src/checkpoint.rs',
  'src/delete.rs',
  'src/element.rs',
  'src/get.rs',
  'src/insert.rs',
  'src/lifecycle.rs',
  'src/proof.rs',
  'src/query.rs',
  'src/transaction.rs',
  'src/types.rs',
)

cargo_toml = files('Cargo.toml')
build_script = files('build.rs')

# CXX bridge generation directory structure
gen_rust_dir = meson.current_build_dir() / 'gen' / 'rust' / 'grovedb_cxx'

if cxxbridge.found()
  # Generate CXX bridge header
  grovedb_cxx_header = custom_target('grovedb_cxx_header',
    output: 'lib.h',
    input: 'src/lib.rs',
    command: [
      'sh', '-c',
      'mkdir -p ' + gen_rust_dir + ' && ' +
      cxxbridge.full_path() + ' @INPUT@ --header -o ' + gen_rust_dir + '/lib.h && ' +
      'cp ' + gen_rust_dir + '/lib.h @OUTPUT@'
    ],
    build_by_default: true,
  )

  # Generate CXX bridge source
  grovedb_cxx_source = custom_target('grovedb_cxx_source',
    output: 'lib.cpp',
    input: 'src/lib.rs',
    command: [
      'sh', '-c',
      'mkdir -p ' + gen_rust_dir + ' && ' +
      cxxbridge.full_path() + ' @INPUT@ -o ' + gen_rust_dir + '/lib.cpp && ' +
      'cp ' + gen_rust_dir + '/lib.cpp @OUTPUT@'
    ],
    build_by_default: true,
  )
else
  # Check for pregenerated files
  gen_source_srcdir = meson.current_source_dir() / 'gen' / 'src' / 'lib.cpp'
  gen_header_srcdir = meson.current_source_dir() / 'gen' / 'include' / 'rust' / 'grovedb_cxx' / 'lib.h'

  if run_command('test', '-f', gen_source_srcdir, check: false).returncode() == 0
    message('Using pregenerated CXX bridge files from source tree')

    grovedb_cxx_header = custom_target('grovedb_cxx_header_copy',
      output: 'lib.h',
      input: gen_header_srcdir,
      command: [
        'sh', '-c',
        'mkdir -p ' + gen_rust_dir + ' && ' +
        'cp @INPUT@ ' + gen_rust_dir + '/lib.h && ' +
        'cp @INPUT@ @OUTPUT@'
      ],
      build_by_default: true,
    )

    grovedb_cxx_source = custom_target('grovedb_cxx_source_copy',
      output: 'lib.cpp',
      input: gen_source_srcdir,
      command: [
        'sh', '-c',
        'mkdir -p ' + gen_rust_dir + ' && ' +
        'cp @INPUT@ ' + gen_rust_dir + '/lib.cpp && ' +
        'cp @INPUT@ @OUTPUT@'
      ],
      build_by_default: true,
    )
  else
    error('cxxbridge not found and no pregenerated bridge files available.\n' +
          'Please install cxxbridge (`cargo install cxxbridge-cmd`) or\n' +
          'generate files first by running cargo build.')
  endif
endif

# Build Rust static library using Cargo
# (Required for grovedb subtree dependencies - external code managed by Cargo)

# Auto-detect Rust host target using rustc
rustc = find_program('rustc', required: true)
rust_host = run_command(rustc, '-vV', check: true).stdout().strip().split('\n')
rust_target = ''
foreach line : rust_host
  if line.startswith('host: ')
    rust_target = line.split(': ')[1]
    break
  endif
endforeach

# Use realpath to resolve symlinks (important for subproject builds)
# When built as a Meson subproject via symlink, we need the real source path for Cargo
realpath = find_program('realpath', required: false)
if realpath.found()
  grovedb_cxx_real_source = run_command(realpath, meson.current_source_dir(), check: true).stdout().strip()
else
  # Fallback for systems without realpath (use Python)
  grovedb_cxx_real_source = run_command('python3', '-c',
    'import os; print(os.path.realpath("' + meson.current_source_dir() + '"))',
    check: true).stdout().strip()
endif

cargo_manifest = grovedb_cxx_real_source / 'Cargo.toml'
cargo_release_flag = cargo_profile == 'release' ? ' --release' : ''
cargo_build_cmd = cargo.full_path() + ' build --manifest-path ' + cargo_manifest + cargo_release_flag

# Cargo outputs to target/<rust-target>/<profile>/
cargo_output_path = grovedb_cxx_real_source / 'target' / rust_target / cargo_profile / 'libgrovedb_cxx.a'

grovedb_cxx_lib = custom_target('grovedb_cxx_staticlib',
  output: 'libgrovedb_cxx.a',
  input: rust_sources + cargo_toml + build_script,
  command: [
    'sh', '-c',
    cargo_build_cmd + ' && ' +
    'cp ' + cargo_output_path + ' @OUTPUT@'
  ],
  build_by_default: true,
  build_always_stale: true,
  console: true,
)

# Declare dependency for downstream projects
grovedb_cxx_dep = declare_dependency(
  sources: [grovedb_cxx_header, grovedb_cxx_source],
  compile_args: ['-I' + meson.current_build_dir() / 'gen'],
  link_with: grovedb_cxx_lib,
)

# Summary
if not meson.is_subproject()
  summary({
    'Cargo profile': cargo_profile,
    'CXX bridge': cxxbridge.found() ? 'generated' : 'pregenerated',
  }, section: 'grovedb_cxx')
endif

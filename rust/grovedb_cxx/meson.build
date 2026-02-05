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

fs = import('fs')
rust = import('rust')

build_script = files(
  'build.rs',
)

cargo_toml = files(
  'Cargo.toml',
)

rust_sources = files(
  'src/lib.rs',
)

# Find required programs
python = find_program('python3')
cxxbridge = find_program('cxxbridge')
cxxbridge_py = '''
import subprocess
import shutil
import sys
from pathlib import Path

cxxbridge, input_file, gen_dir, output_file, *extra_args = sys.argv[1:]
gen_path = Path(gen_dir)
gen_path.mkdir(parents=True, exist_ok=True)

# Generate to gen directory
gen_file = gen_path / Path(output_file).name
subprocess.run([cxxbridge, input_file, '-o', str(gen_file)] + extra_args, check=True)

# Copy to Meson output location
shutil.copy2(gen_file, output_file)
'''

gen_rust_dir = meson.current_build_dir() / 'gen' / 'rust' / 'grovedb_cxx'
grovedb_cxx_header = custom_target('grovedb_cxx_header',
  output: 'lib.h',
  input: 'src/lib.rs',
  command: [
    python, '-c', cxxbridge_py,
    cxxbridge.full_path(), '@INPUT@', gen_rust_dir, '@OUTPUT@', '--header'
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('includedir') / 'rust' / 'grovedb_cxx',
)
grovedb_cxx_source = custom_target('grovedb_cxx_source',
  output: 'lib.cpp',
  input: 'src/lib.rs',
  command: [
    python, '-c', cxxbridge_py,
    cxxbridge.full_path(), '@INPUT@', gen_rust_dir, '@OUTPUT@'
  ],
  build_by_default: true,
)

# Auto-detect Rust host triple
rustc = find_program('rustc')
rust_host = run_command(rustc, '-vV', check: true).stdout().strip().split('\n')
rust_target = ''
foreach line : rust_host
  if line.startswith('host: ')
    rust_target = line.split(': ')[1]
    break
  endif
endforeach

# Resolve current_source_dir if symlinked for cargo
grovedb_cxx_real_source = run_command(
  python,
  '-c',
  'import os; print(os.path.realpath("@0@"))'.format(meson.current_source_dir()),
  check: true
).stdout().strip()
cargo_manifest = grovedb_cxx_real_source / 'Cargo.toml'
cargo_target_dir = grovedb_cxx_real_source / 'target'

# Construct 'cargo' invocation
cargo = find_program('cargo')
cargo_py = '''
import subprocess
import shutil
import sys
from pathlib import Path

cargo_cmd, source_lib, dest_lib, *cargo_args = sys.argv[1:]

# Build with cargo
subprocess.run([cargo_cmd] + cargo_args, check=True)

# Copy to Meson output location
Path(dest_lib).parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(source_lib, dest_lib)
'''
cargo_args = [
  'build',
  '--manifest-path', cargo_manifest,
  '--target-dir', cargo_target_dir,
  '--target', rust_target,
]

# Set profile
cargo_profile = get_option('cargo_profile')
cargo_output_path = cargo_target_dir / rust_target / cargo_profile / 'libgrovedb_cxx.a'
if cargo_profile == 'release'
  cargo_args += '--release'
endif

# Build library
grovedb_cxx_lib = custom_target('grovedb_cxx_staticlib',
  output: 'libgrovedb_cxx.a',
  input: rust_sources + cargo_toml + build_script,
  command: [
    python, '-c', cargo_py,
    cargo.full_path(),
    cargo_output_path,
    '@OUTPUT@',
  ] + cargo_args,
  build_by_default: true,
  build_always_stale: true,
  console: true,
)

grovedb_cxx_dep = declare_dependency(
  sources: [grovedb_cxx_header, grovedb_cxx_source],
  compile_args: ['-I' + meson.current_build_dir() / 'gen'],
  link_with: grovedb_cxx_lib,
)

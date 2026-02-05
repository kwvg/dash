project('rustdeps',
  'rust',
  version: '1.0.0',
  meson_version: '>=1.9.0',
  default_options: [
    'warning_level=3',
    'rust_std=2018',
  ]
)

rust_libs = []
rust_lib_names = []
rust_sources = []
rust_compile_args = []

with_grovedb_cxx = get_option('with_grovedb_cxx')
if with_grovedb_cxx
  message('Building with grovedb_cxx')
  grovedb_cxx_subproj = subproject('grovedb_cxx', default_options: ['cargo_profile=' + get_option('cargo_profile')])
  grovedb_cxx_dep = grovedb_cxx_subproj.get_variable('grovedb_cxx_dep')
  rust_libs += [grovedb_cxx_subproj.get_variable('grovedb_cxx_lib')]
  rust_lib_names += ['grovedb_cxx']
  rust_sources += [grovedb_cxx_subproj.get_variable('grovedb_cxx_header'), grovedb_cxx_subproj.get_variable('grovedb_cxx_source')]
  rust_compile_args += ['-I' + meson.current_build_dir() / 'subprojects' / 'grovedb_cxx' / 'gen']
endif

# Create librustdeps.a aggregate
if rust_libs.length() == 0
  error('No Rust crates enabled. Enable at least one with -Dwith_<crate>=true')
endif

# Resolve archiver from environment or assume default
python = find_program('python3')
ar_cmd = run_command(
  python,
  '-c',
  'import os; print(os.environ.get("AR", "ar"))',
  check: true
).stdout().strip()
ar = find_program(ar_cmd)

# Library amalgamation routine for merging crate libraries
merge_archives_py = '''
import subprocess
import sys
import tempfile
import shutil
from pathlib import Path

ar_cmd = sys.argv[1]
output_archive = sys.argv[2]
input_archives = sys.argv[3:]

if len(input_archives) < 1:
    print("Error: At least one input archive required", file=sys.stderr)
    sys.exit(1)

# Create temp directory
with tempfile.TemporaryDirectory() as temp_dir:
    temp_path = Path(temp_dir)

    # Extract all input archives
    for lib in input_archives:
        lib_path = Path(lib).resolve()
        subprocess.run([ar_cmd, 'x', str(lib_path)], cwd=temp_path, check=True)

    # Get all .o files
    obj_files = list(temp_path.glob('*.o'))
    if not obj_files:
        print("Error: No object files found in archives", file=sys.stderr)
        sys.exit(1)

    # Create output archive
    output_path = Path(output_archive).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([ar_cmd, 'rcs', str(output_path)] + [str(o) for o in obj_files],
                   cwd=temp_path, check=True)
'''
rustdeps_lib = custom_target('librustdeps',
  output: 'librustdeps.a',
  input: rust_libs,
  command: [python, '-c', merge_archives_py, ar.full_path(), '@OUTPUT@', '@INPUT@'],
  build_by_default: true,
)
rustdeps_dep = declare_dependency(
  sources: rust_sources,
  compile_args: rust_compile_args,
  link_with: rustdeps_lib,
)

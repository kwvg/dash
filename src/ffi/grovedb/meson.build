# SPDX-FileCopyrightText: 2026 The Dash Core developers
# SPDX-License-Identifier: MIT

project('libgrovedb',
  ['c', 'cpp'],
  version: '1.0.0',
  license: 'MIT',
  meson_version: '>=1.9.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=false',
  ]
)

project_description = 'A GroveDB C++ library using FFI bindings'

# Build options
build_tests = get_option('build_tests')
use_rustdeps = get_option('use_rustdeps')
grovedb_cxx_build_dir = get_option('grovedb_cxx_build_dir')
rustdeps_build_dir = get_option('rustdeps_build_dir')

# C++ compiler
cxx = meson.get_compiler('cpp')

# Check for threading support
thread_dep = dependency('threads', required: true)

# Find or link to Rust libraries
# Two modes:
# 1. Link against librustdeps (contains grovedb_cxx) - use_rustdeps=true
# 2. Link against libgrovedb_cxx directly - use_rustdeps=false

if use_rustdeps
  # Mode 1: Use librustdeps (aggregated Rust components)
  if rustdeps_build_dir != ''
    rust_build_dir = rustdeps_build_dir
  else
    rust_build_dir = meson.project_source_root() / '..' / '..' / '..' / 'rust' / 'builddir'
  endif

  if run_command('test', '-d', rust_build_dir, check: false).returncode() != 0
    error('librustdeps not built yet. Please build it first:\n' +
          '  cd ' + meson.project_source_root() / '..' / '..' / '..' / 'rust\n' +
          '  meson setup builddir -Dwith_grovedb_cxx=true\n' +
          '  meson compile -C builddir')
  endif

  message('Using librustdeps from:', rust_build_dir)

  # With native Meson Rust support, libraries are in standard Meson locations
  grovedb_cxx_include = rust_build_dir / 'grovedb_cxx' / 'gen'
  grovedb_cxx_bridge_source = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
  grovedb_cxx_bridge_header = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

  # Find the Rust library (libgrovedb_cxx.a is in grovedb_cxx/ subdirectory)
  grovedb_cxx_lib = declare_dependency(
    compile_args: ['-I' + grovedb_cxx_include],
    dependencies: cxx.find_library('grovedb_cxx', dirs: [rust_build_dir / 'grovedb_cxx']),
  )

  cxx_bridge_lib = static_library('cxxbridge_grovedb',
    grovedb_cxx_bridge_source,
    cpp_args: ['-std=c++20', '-I' + grovedb_cxx_include],
  )

elif grovedb_cxx_build_dir != ''
  # Mode 2: Use user-provided grovedb_cxx build directory directly
  message('Using grovedb_cxx from:', grovedb_cxx_build_dir)

  grovedb_cxx_include = grovedb_cxx_build_dir / 'gen'
  grovedb_cxx_bridge_source = grovedb_cxx_build_dir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
  grovedb_cxx_bridge_header = grovedb_cxx_build_dir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

  if not run_command('test', '-f', grovedb_cxx_bridge_source, check: false).returncode() == 0
    error('CXX bridge source not found at: ' + grovedb_cxx_bridge_source)
  endif

  grovedb_cxx_lib = declare_dependency(
    compile_args: ['-I' + grovedb_cxx_include],
    dependencies: cxx.find_library('grovedb_cxx', dirs: [grovedb_cxx_build_dir]),
  )

  cxx_bridge_lib = static_library('cxxbridge_grovedb',
    grovedb_cxx_bridge_source,
    cpp_args: ['-std=c++20', '-I' + grovedb_cxx_include],
  )
else
  # Try to find grovedb_cxx in ../../../rust/grovedb_cxx
  grovedb_cxx_dir = meson.current_source_dir() / '..' / '..' / '..' / 'rust' / 'grovedb_cxx'
  grovedb_cxx_build_dir = meson.current_build_dir() / '..' / '..' / '..' / 'rust' / 'builddir' / 'grovedb_cxx'

  # Check if we can build grovedb_cxx
  if run_command('test', '-f', grovedb_cxx_dir / 'meson.build', check: false).returncode() == 0
    message('Found grovedb_cxx, will look for built artifacts')

    # Check for built grovedb_cxx (relative to source root)
    rust_build_dir = meson.project_source_root() / '..' / '..' / '..' / 'rust' / 'builddir'

    if run_command('test', '-d', rust_build_dir, check: false).returncode() != 0
      error('grovedb_cxx not built yet. Please build it first:\n' +
            '  cd ' + meson.project_source_root() / '..' / '..' / '..' / 'rust\n' +
            '  meson setup builddir\n' +
            '  meson compile -C builddir')
    endif

    # Use the pre-built grovedb_cxx
    grovedb_cxx_include = rust_build_dir / 'grovedb_cxx' / 'gen'
    grovedb_cxx_bridge_source = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
    grovedb_cxx_bridge_header = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

    if run_command('test', '-f', grovedb_cxx_bridge_source, check: false).returncode() != 0
      error('grovedb_cxx library not found.\n' +
            'Please build grovedb_cxx first (see above)')
    endif

    grovedb_cxx_lib = declare_dependency(
      compile_args: ['-I' + grovedb_cxx_include],
      dependencies: cxx.find_library('grovedb_cxx', dirs: [rust_build_dir / 'grovedb_cxx']),
    )

    # Compile the CXX bridge source
    cxx_bridge_lib = static_library('cxxbridge_grovedb',
      grovedb_cxx_bridge_source,
      cpp_args: ['-std=c++20', '-I' + grovedb_cxx_include],
    )
  else
    error('Cannot find grovedb_cxx. Please either:\n' +
          '1. Build grovedb_cxx and specify -Dgrovedb_cxx_build_dir=<path>\n' +
          '2. Ensure ../../../rust/grovedb_cxx/meson.build exists')
  endif
endif

# libgrovedb source files
libgrovedb_sources = files(
  'src/auxiliary.cpp',
  'src/batch.cpp',
  'src/checkpoint.cpp',
  'src/db.cpp',
  'src/delete.cpp',
  'src/element.cpp',
  'src/get.cpp',
  'src/proof.cpp',
  'src/put.cpp',
  'src/query.cpp',
  'src/transaction.cpp',
  'src/util/wire_read.cpp',
  'src/util/wire_write.cpp',
)

# Public headers
libgrovedb_headers = files(
  'include/grovedb/batch.h',
  'include/grovedb/cost.h',
  'include/grovedb/db.h',
  'include/grovedb/element.h',
  'include/grovedb/proof.h',
  'include/grovedb/query.h',
  'include/grovedb/status.h',
  'include/grovedb/transaction.h',
  'include/grovedb/types.h',
  'include/grovedb/wire.h',
)

# Include directories
libgrovedb_inc = include_directories('include', 'src')

# Generate config header
conf_data = configuration_data()
conf_data.set('PACKAGE_NAME', meson.project_name())
conf_data.set('PACKAGE_VERSION', meson.project_version())

configure_file(
  output: 'grovedb-config.h',
  configuration: conf_data,
  install: false,
)

# Build shared library
libgrovedb_shared = shared_library('grovedb',
  libgrovedb_sources,
  include_directories: libgrovedb_inc,
  dependencies: [grovedb_cxx_lib, thread_dep],
  link_with: cxx_bridge_lib,
  cpp_args: ['-DHAVE_GROVEDB_CONFIG'],
  install: true,
  version: meson.project_version(),
  soversion: '1',
)

# Build static library
libgrovedb_static = static_library('grovedb',
  libgrovedb_sources,
  include_directories: libgrovedb_inc,
  dependencies: [grovedb_cxx_lib, thread_dep],
  link_with: cxx_bridge_lib,
  cpp_args: ['-DHAVE_GROVEDB_CONFIG'],
  install: true,
)

# Install headers
install_headers(libgrovedb_headers, subdir: 'grovedb')

# Declare dependency for downstream projects
libgrovedb_dep = declare_dependency(
  include_directories: libgrovedb_inc,
  link_with: libgrovedb_shared,
  dependencies: [grovedb_cxx_lib, thread_dep],
)

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  libgrovedb_shared,
  description: project_description,
  name: 'libgrovedb',
  filebase: 'libgrovedb',
  subdirs: 'grovedb',
)

# Tests
if build_tests
  boost_dep = dependency('boost',
    modules: ['unit_test_framework'],
    version: '>=1.73.0',
    required: false
  )

  if boost_dep.found()
    test_sources = files(
      'src/test/auxiliary_tests.cpp',
      'src/test/batch_tests.cpp',
      'src/test/checkpoint_tests.cpp',
      'src/test/delete_tests.cpp',
      'src/test/get_tests.cpp',
      'src/test/insert_tests.cpp',
      'src/test/lifecycle_tests.cpp',
      'src/test/main.cpp',
      'src/test/polyfill_tests.cpp',
      'src/test/proof_tests.cpp',
      'src/test/query_tests.cpp',
      'src/test/transaction_tests.cpp',
      'src/test/util/tempdir.cpp',
      'src/test/whoami.cpp',
      'src/test/wire_tests.cpp',
    )

    test_exe = executable('test_grovedb',
      test_sources,
      include_directories: libgrovedb_inc,
      dependencies: [libgrovedb_dep, boost_dep],
      link_args: ['-static'],
      build_by_default: true,
    )

    test('grovedb_tests', test_exe, protocol: 'boost')
  else
    warning('Boost not found. Tests will not be built.')
  endif
endif

# Summary
summary({
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
  'Build tests': build_tests,
  'Boost available': build_tests ? boost_dep.found() : false,
}, section: 'Configuration')

summary({
  'C++ compiler': cxx.get_id(),
  'C++ standard': get_option('cpp_std'),
}, section: 'Compiler')

summary({
  'Shared library': libgrovedb_shared.full_path(),
  'Static library': libgrovedb_static.full_path(),
}, section: 'Build Output')

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
    'werror=true',  # Default to werror=true to match enable-werror default
    'b_lundef=false',  # Disable linker undefined checks (incompatible with sanitizers)
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

# Configure compiler flags (warnings, hardening, sanitizers, coverage)
# Based on Dash Core configure.ac

# Get options
enable_werror = get_option('enable-werror')
enable_hardening = get_option('enable-hardening')
enable_suppress_external = get_option('enable-suppress-external-warnings')
enable_lcov = get_option('enable-lcov')
enable_lcov_branch = get_option('enable-lcov-branch-coverage')

# Initialize flag arrays
warn_flags = []
error_flags = []
nowarn_flags = []
hardening_compile_flags = []
hardening_link_flags = []
sanitizer_flags = []
coverage_flags = []

# Check if werror is supported
werror_flag = []
if cxx.has_argument('-Werror')
  werror_flag = ['-Werror']
endif

# Warning flags (from Dash Core configure.ac)
# Note: -Wall and -Wextra are handled by warning_level=3, skip them to avoid Meson warnings
if cxx.has_argument('-Wgnu')
  warn_flags += '-Wgnu'
endif
# -Wformat and -Wformat-security must be combined
if cxx.has_multi_arguments(['-Wformat', '-Wformat-security'])
  warn_flags += ['-Wformat', '-Wformat-security']
endif
if cxx.has_argument('-Wreorder')
  warn_flags += '-Wreorder'
endif
if cxx.has_argument('-Wvla')
  warn_flags += '-Wvla'
endif
if cxx.has_argument('-Wshadow-field')
  warn_flags += '-Wshadow-field'
endif
if cxx.has_argument('-Wthread-safety')
  warn_flags += '-Wthread-safety'
endif
if cxx.has_argument('-Wloop-analysis')
  warn_flags += '-Wloop-analysis'
endif
if cxx.has_argument('-Wredundant-decls')
  warn_flags += '-Wredundant-decls'
endif
if cxx.has_argument('-Wunused-member-function')
  warn_flags += '-Wunused-member-function'
endif
if cxx.has_argument('-Wdate-time')
  warn_flags += '-Wdate-time'
endif
if cxx.has_argument('-Wconditional-uninitialized')
  warn_flags += '-Wconditional-uninitialized'
endif
if cxx.has_argument('-Wduplicated-branches')
  warn_flags += '-Wduplicated-branches'
endif
if cxx.has_argument('-Wduplicated-cond')
  warn_flags += '-Wduplicated-cond'
endif

# Conditional no-warn flags
if enable_werror
  if cxx.get_id() == 'clang' and cxx.has_argument('-Wdeprecated-literal-operator')
    nowarn_flags += '-Wno-error=deprecated-literal-operator'
  endif

  if cxx.get_id() == 'gcc'
    gcc_version = cxx.version().split('.')
    if gcc_version[0].to_int() < 14 and cxx.has_argument('-Wattributes')
      nowarn_flags += '-Wno-error=attributes'
    endif
    if gcc_version[0].to_int() < 12 and cxx.has_argument('-Wcpp')
      nowarn_flags += '-Wno-error=cpp'
    endif
  endif
endif

# Suppress problematic GCC warnings entirely
if cxx.get_id() == 'gcc'
  foreach flag : ['-Warray-bounds', '-Wstringop-overread', '-Wstringop-overflow']
    if cxx.has_argument(flag)
      nowarn_flags += '-Wno' + flag.substring(2)
    endif
  endforeach
endif

# Error flags - use Meson's built-in werror option instead of manual -Werror
# The enable-werror option is kept for configure.ac compatibility but maps to b_werror
if enable_werror
  # Meson's b_werror is set via default_options or command line -Dwerror=true
  # We just validate it's consistent here
  if not get_option('werror')
    warning('enable-werror=true but werror=false. Setting werror=true is recommended.')
  endif
endif

# Hardening flags
if enable_hardening
  if cxx.has_argument('-Wstack-protector')
    hardening_compile_flags += '-Wstack-protector'
  endif
  if cxx.has_argument('-fstack-protector-all')
    hardening_compile_flags += '-fstack-protector-all'
  endif
  if cxx.has_argument('-fcf-protection=full')
    hardening_compile_flags += '-fcf-protection=full'
  endif

  if host_machine.system() != 'windows'
    if cxx.has_argument('-fstack-clash-protection')
      hardening_compile_flags += '-fstack-clash-protection'
    endif
  endif

  if host_machine.cpu_family() == 'aarch64'
    if cxx.has_argument('-mbranch-protection=bti')
      hardening_compile_flags += '-mbranch-protection=bti'
    endif
  endif

  if get_option('buildtype') != 'debug'
    hardening_compile_flags += ['-U_FORTIFY_SOURCE', '-D_FORTIFY_SOURCE=3']
  endif

  if cxx.has_link_argument('-fPIE')
    hardening_compile_flags += '-fPIE'
    if cxx.has_link_argument('-pie')
      hardening_link_flags += '-pie'
    endif
  endif

  if host_machine.system() == 'windows'
    foreach flag : ['--enable-reloc-section', '--dynamicbase', '--nxcompat', '--high-entropy-va']
      link_flag = '-Wl,' + flag
      if cxx.has_link_argument(link_flag)
        hardening_link_flags += link_flag
      endif
    endforeach
  elif host_machine.system() == 'darwin'
    foreach flag : ['-Wl,-fixup_chains']
      if cxx.has_link_argument(flag)
        hardening_link_flags += flag
      endif
    endforeach
  else
    foreach flag : ['-Wl,-z,relro', '-Wl,-z,now', '-Wl,-z,separate-code']
      if cxx.has_link_argument(flag)
        hardening_link_flags += flag
      endif
    endforeach
  endif
endif

# Coverage flags
if enable_lcov
  lcov = find_program('lcov', required: true)
  genhtml = find_program('genhtml', required: true)

  if cxx.has_argument('--coverage')
    coverage_flags = ['--coverage']
  else
    error('lcov requested but --coverage flag not supported')
  endif
endif

# Apply all flags
compile_flags = warn_flags + nowarn_flags + error_flags + hardening_compile_flags + coverage_flags
link_flags = hardening_link_flags + coverage_flags

add_project_arguments(compile_flags, language: 'cpp')
add_project_link_arguments(link_flags, language: 'cpp')

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
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())

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
  boost_dep_raw = dependency('boost',
    modules: ['unit_test_framework'],
    version: '>=1.73.0',
    required: false,
    include_type: enable_suppress_external ? 'system' : 'preserve'
  )

  # Wrap boost dependency to ensure system includes when suppressing external warnings
  if boost_dep_raw.found()
    boost_dep = boost_dep_raw
  else
    boost_dep = boost_dep_raw
  endif

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

    # Build test executable
    # Note: -static doesn't work on macOS, so only use it on Linux
    test_link_args = []
    if host_machine.system() == 'linux'
      test_link_args = ['-static']
    endif

    test_exe = executable('test_grovedb',
      test_sources,
      include_directories: libgrovedb_inc,
      dependencies: [libgrovedb_dep, boost_dep],
      link_args: test_link_args,
      build_by_default: true,
    )

    test('grovedb_tests', test_exe)
  else
    warning('Boost not found. Tests will not be built.')
  endif
endif

# Fuzz targets
build_fuzz = get_option('build_fuzz')
if build_fuzz
  fuzz_util_sources = files(
    'src/fuzz/utils/tempdir.cpp',
  )

  fuzz_inc = include_directories('src/fuzz', 'src/fuzz/utils')

  fuzz_targets = [
    'fuzz_element',
  ]

  foreach target : fuzz_targets
    exe = executable(target,
      'src/fuzz/@0@.cpp'.format(target),
      fuzz_util_sources,
      include_directories: [libgrovedb_inc, fuzz_inc],
      dependencies: [libgrovedb_dep],
      build_by_default: true,
    )
  endforeach

  summary({'Fuzz targets': fuzz_targets.length()}, section: 'Fuzz')
endif

# Summary
summary({
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
  'Build tests': build_tests,
  'Build fuzz targets': build_fuzz,
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

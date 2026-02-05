project('libgrovedb',
  ['c', 'cpp'],
  version: '1.0.0',
  license: 'MIT',
  meson_version: '>=1.9.0',
  default_options: [
    'b_staticpic=true',
    'cpp_std=c++20',
    'prefix=/usr/local',
    'warning_level=3',
    'werror=true',
  ]
)

fs = import('fs')

cxx = meson.get_compiler('cpp')
thread_dep = dependency('threads', required: true)

libgrovedb_sources = files(
  'src/misc.cpp',
)

libgrovedb_headers = files(
  'include/grovedb/grovedb.h',
  'include/grovedb/misc.h',
)

libgrovedb_includes = include_directories('include', 'src')

rustdeps_build_dir = get_option('rustdeps_build_dir')
grovedb_cxx_build_dir = get_option('grovedb_cxx_build_dir')
use_rustdeps = get_option('use_rustdeps')

project_root = meson.project_source_root()
repo_root = fs.parent(fs.parent(fs.parent(project_root)))
rust_dir = repo_root / 'rust'

if use_rustdeps
  if rustdeps_build_dir != ''
    rust_build_dir = rustdeps_build_dir
  else
    rust_build_dir = rust_dir / 'builddir'
  endif

  message('Using librustdeps from:', rust_build_dir)

  if not fs.is_dir(rust_build_dir)
    error('librustdeps not built yet at: @0@\n'.format(rust_build_dir) +
          'Please build it first:\n' +
          '  cd @0@\n'.format(rust_dir) +
          '  meson setup builddir -Dwith_grovedb_cxx=true\n' +
          '  meson compile -C builddir')
  endif

  grovedb_cxx_subdir = rust_build_dir / 'subprojects' / 'grovedb_cxx'
  grovedb_cxx_include = grovedb_cxx_subdir / 'gen'
  grovedb_cxx_bridge_source = grovedb_cxx_subdir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
  grovedb_cxx_bridge_header = grovedb_cxx_subdir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

  if not fs.is_file(grovedb_cxx_bridge_source)
    error('CXX bridge source not found at: @0@\n'.format(grovedb_cxx_bridge_source) +
          'librustdeps may not have been built with -Dwith_grovedb_cxx=true')
  endif

  grovedb_cxx_lib = declare_dependency(
    compile_args: ['-I' + grovedb_cxx_include],
    dependencies: cxx.find_library('grovedb_cxx', dirs: [grovedb_cxx_subdir]),
  )

  cxx_bridge_lib = static_library('cxxbridge_grovedb',
    grovedb_cxx_bridge_source,
    cpp_args: ['-I' + grovedb_cxx_include],
  )
elif grovedb_cxx_build_dir != ''
  message('Using grovedb_cxx from:', grovedb_cxx_build_dir)

  if not fs.is_absolute(grovedb_cxx_build_dir)
    grovedb_cxx_build_dir = meson.current_source_dir() / grovedb_cxx_build_dir
  endif

  grovedb_cxx_include = grovedb_cxx_build_dir / 'gen'
  grovedb_cxx_bridge_source = grovedb_cxx_build_dir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
  grovedb_cxx_bridge_header = grovedb_cxx_build_dir / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

  if not fs.is_file(grovedb_cxx_bridge_source)
    error('CXX bridge source not found at: @0@\n'.format(grovedb_cxx_bridge_source) +
          'Please ensure grovedb_cxx is built at the specified path')
  endif

  grovedb_cxx_lib = declare_dependency(
    compile_args: ['-I' + grovedb_cxx_include],
    dependencies: cxx.find_library('grovedb_cxx', dirs: [grovedb_cxx_build_dir]),
  )

  cxx_bridge_lib = static_library('cxxbridge_grovedb',
    grovedb_cxx_bridge_source,
    cpp_args: ['-I' + grovedb_cxx_include],
  )
else
  grovedb_cxx_source_dir = rust_dir / 'grovedb_cxx'
  grovedb_cxx_build_dir = rust_dir / 'builddir' / 'grovedb_cxx'

  grovedb_cxx_meson_build = grovedb_cxx_source_dir / 'meson.build'
  if not fs.is_file(grovedb_cxx_meson_build)
    error('Cannot find grovedb_cxx source at: @0@\n'.format(grovedb_cxx_source_dir) +
          'Please either:\n' +
          '1. Build grovedb_cxx and specify -Dgrovedb_cxx_build_dir=<path>\n' +
          '2. Ensure grovedb_cxx exists at @0@'.format(grovedb_cxx_source_dir))
  else
    message('Found grovedb_cxx source, looking for built artifacts')
  endif

  rust_build_dir = rust_dir / 'builddir'
  if not fs.is_dir(rust_build_dir)
    error('grovedb_cxx not built yet. Build directory not found at: @0@\n'.format(rust_build_dir) +
          'Please build it first:\n' +
          '  cd @0@\n'.format(rust_dir) +
          '  meson setup builddir\n' +
          '  meson compile -C builddir')
  endif

  grovedb_cxx_include = rust_build_dir / 'grovedb_cxx' / 'gen'
  grovedb_cxx_bridge_source = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.cpp'
  grovedb_cxx_bridge_header = rust_build_dir / 'grovedb_cxx' / 'gen' / 'rust' / 'grovedb_cxx' / 'lib.h'

  if not fs.is_file(grovedb_cxx_bridge_source)
    error('CXX bridge source not found at: @0@\n'.format(grovedb_cxx_bridge_source) +
          'grovedb_cxx appears not to be built. Please run:\n' +
          '  cd @0@\n'.format(rust_dir) +
          '  meson compile -C builddir')
  endif

  grovedb_cxx_lib = declare_dependency(
    compile_args: ['-I' + grovedb_cxx_include],
    dependencies: cxx.find_library('grovedb_cxx', dirs: [rust_build_dir / 'grovedb_cxx']),
  )

  cxx_bridge_lib = static_library('cxxbridge_grovedb',
    grovedb_cxx_bridge_source,
    cpp_args: ['-I' + grovedb_cxx_include],
  )
endif

shared_library = get_option('shared_library')
build_tests = get_option('build_tests')
if build_tests and not shared_library
  warn('Tests enabled, building shared library')
endif

needs_pkgconfig = (shared_library or build_tests)
lib_type = needs_pkgconfig ? 'shared' : 'static'
if needs_pkgconfig
  libgrovedb = shared_library('grovedb',
    libgrovedb_sources,
    include_directories: libgrovedb_includes,
    dependencies: [grovedb_cxx_lib, thread_dep],
    link_with: cxx_bridge_lib,
    install: true,
    version: meson.project_version(),
    soversion: '1',
  )
else
  libgrovedb = static_library('grovedb',
    libgrovedb_sources,
    include_directories: libgrovedb_includes,
    dependencies: [grovedb_cxx_lib, thread_dep],
    link_with: cxx_bridge_lib,
    install: true,
  )
endif

libgrovedb_dep = declare_dependency(
  include_directories: libgrovedb_includes,
  link_with: libgrovedb,
  dependencies: [grovedb_cxx_lib, thread_dep],
)

if needs_pkgconfig
  pkgconfig = import('pkgconfig')
  pkgconfig.generate(
    libgrovedb,
    description: 'C++20 bindings for GroveDb',
    name: 'libgrovedb',
    filebase: 'libgrovedb',
    subdirs: 'grovedb',
  )
endif

# Unit tests
if build_tests
  boost_dep = dependency('boost',
    modules: ['unit_test_framework'],
    version: '>=1.73.0',
  )

  test_sources = files(
    'src/test/main.cpp',
    'src/test/misc_tests.cpp',
  )

  # Build test executable
  test_exe = executable('test_grovedb',
    test_sources,
    include_directories: libgrovedb_includes,
    dependencies: [libgrovedb_dep, boost_dep],
    build_by_default: true,
  )

  test('grovedb_tests', test_exe)
endif

install_headers(libgrovedb_headers, subdir: 'grovedb')

summary({
  'C++ compiler': cxx.get_id(),
  'C++ standard': get_option('cpp_std'),
}, section: 'Compiler')

summary({
  'Build tests': build_tests,
  'Library type': lib_type,
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
}, section: 'Configuration')

rustdeps_build_dir = get_option('rustdeps_build_dir')
grovedb_cxx_build_dir = get_option('grovedb_cxx_build_dir')
use_rustdeps = get_option('use_rustdeps')

repo_root = fs.parent(fs.parent(fs.parent(meson.project_source_root())))
rust_dir = repo_root / 'rust'

if use_rustdeps
  # Mode 1: Link against librustdeps (aggregated Rust components)
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
  # Mode 2: Link against user-specified grovedb_cxx build directory
  message('Using grovedb_cxx from:', grovedb_cxx_build_dir)

  if not fs.is_absolute(grovedb_cxx_build_dir)
    grovedb_cxx_build_dir = meson.project_source_root() / grovedb_cxx_build_dir
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
  # Mode 3: Auto-detect grovedb_cxx from sibling rust directory
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

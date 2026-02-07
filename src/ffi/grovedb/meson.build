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

build_bench = get_option('build_bench')
build_examples = get_option('build_examples')
build_fuzz = get_option('build_fuzz')
if build_fuzz
  subdir('build-aux/fuzz')
endif

libgrovedb_sources = files(
  'src/db.cpp',
  'src/delete.cpp',
  'src/element.cpp',
  'src/get.cpp',
  'src/put.cpp',
  'src/transaction.cpp',
  'src/types.cpp',
  'src/wire_read.cpp',
  'src/wire_write.cpp',
)

libgrovedb_vendor_headers = files(
  'include/grovedb/vendor/tl/expected.hpp',
)

libgrovedb_headers = files(
  'include/grovedb/cost.h',
  'include/grovedb/costed.h',
  'include/grovedb/db.h',
  'include/grovedb/element.h',
  'include/grovedb/error.h',
  'include/grovedb/result.h',
  'include/grovedb/transaction.h',
  'include/grovedb/types.h',
  'include/grovedb/wire.h',
)

libgrovedb_includes = include_directories('include', 'src')

cxx_flag_error = '-Werror'
if host_machine.system() == 'darwin'
  cxx_lflag_error = '-Wl,-fatal_warnings'
else
  cxx_lflag_error = '-Wl,--fatal_warnings'
endif

cxx_core_flags_list = [
  '-fno-extended-identifiers',
  '-fstack-reuse=none', # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348
]

cxx_harden_flags_list = [
  '-fcf-protection=full',
  '-fstack-protector-all',
  '-Wstack-protector',
]

cxx_warn_flags_list = [
  '-Wconditional-uninitialized',
  '-Wdate-time',
  '-Wdocumentation',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wgnu',
  '-Wimplicit-fallthrough',
  '-Wlogical-op',
  '-Wloop-analysis',
  '-Woverloaded-virtual',
  '-Wredundant-decls',
  '-Wreorder',
  '-Wself-assign',
  '-Wshadow-field',
  '-Wsuggest-override',
  '-Wthread-safety',
  '-Wunreachable-code',
  '-Wunused-member-function',
  '-Wvla',
]

# C++ linker flags
cxx_harden_lflags_darwin_list = [
  '-dead_strip',
  '-dead_strip_dylibs',
  '-fixup_chains',
  '-headerpad_max_install_names',
]

cxx_harden_lflags_linux_list = [
  '-z,now',
  '-z,relro',
  '-z,separate-code',
]

cxx_harden_lflags_win64_list = [
  '--dynamicbase',
  '--enable-reloc-section',
  '--high-entropy-va',
  '--nxcompat',
]

# C++ preprocessor flags
cxx_harden_release_preproc = [
  '-D_FORTIFY_SOURCE=3',
  '-U_FORTIFY_SOURCE',
]

# Check compiler support for flags and apply them
cxx_flags = []

foreach flag : cxx_core_flags_list + cxx_warn_flags_list
  if cxx.has_multi_arguments([cxx_flag_error, flag])
    cxx_flags += flag
  endif
endforeach

# -Wformat and -Wformat-security must defined together
if cxx.has_multi_arguments([cxx_flag_error, '-Wformat', '-Wformat-security'])
  cxx_flags += ['-Wformat', '-Wformat-security']
endif

harden_build = get_option('harden_build')
if harden_build
  foreach flag : cxx_harden_flags_list
    if cxx.has_multi_arguments([cxx_flag_error, flag])
      cxx_flags += flag
    endif
  endforeach

  if host_machine.cpu_family() == 'aarch64'
    if cxx.has_multi_arguments(cxx_flag_error, '-mbranch-protection=bti')
      cxx_flags += '-mbranch-protection=bti'
    endif
  endif

  # -fstack-clash-protection is a no-op on windows, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90458
  if host_machine.system() != 'windows'
    if cxx.has_multi_arguments(cxx_flag_error, '-fstack-clash-protection')
      cxx_flags += '-fstack-clash-protection'
    endif
  endif
endif

cxx_lflags = []

if harden_build
  cxx_harden_lflags = []
  if host_machine.system() == 'darwin'
    cxx_harden_lflags = cxx_harden_lflags_darwin_list
  elif host_machine.system() == 'windows'
    cxx_harden_lflags = cxx_harden_lflags_win64_list
  else
    cxx_harden_lflags = cxx_harden_lflags_linux_list
  endif
  foreach flag : cxx_harden_lflags
    flag = '-Wl,' + flag
    if cxx.has_multi_link_arguments([cxx_lflag_error, flag])
      cxx_lflags += flag
    endif
  endforeach
endif

cxx_preproc = []

if harden_build
  if get_option('buildtype') != 'debug'
    foreach flag : cxx_harden_release_preproc
      cxx_preproc += flag
    endforeach
  endif
endif

add_project_arguments(cxx_flags + cxx_preproc, language: 'cpp')
add_project_link_arguments(cxx_lflags, language: 'cpp')

subdir('build-aux/ffi')

build_tests = get_option('build_tests')
shared_library = get_option('shared_library')
if not shared_library
  if build_bench
    warn('Benchmarks enabled, building shared library')
  elif build_examples
    warn('Examples enabled, building shared library')
  elif build_fuzz
    warn('Fuzzing enabled, building shared library')
  elif build_tests
    warn('Tests enabled, building shared library')
  endif
endif

needs_pkgconfig = (shared_library or build_bench or build_examples or build_fuzz or build_tests)
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
suppress_external_warns = get_option('suppress_external_warnings')
if build_tests
  boost_dep = dependency('boost',
    modules: ['unit_test_framework'],
    version: '>=1.73.0',
    include_type: suppress_external_warns ? 'system' : 'preserve',
  )

  test_sources = files(
    'src/test/main.cpp',
    'src/test/delete_tests.cpp',
    'src/test/element_tests.cpp',
    'src/test/get_tests.cpp',
    'src/test/insert_tests.cpp',
    'src/test/lifecycle_tests.cpp',
    'src/test/transaction_tests.cpp',
    'src/test/util/tempdir.cpp',
    'src/test/wire_tests.cpp',
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
install_headers(libgrovedb_vendor_headers, subdir: 'grovedb/vendor/tl')

# Benchmarks
if build_bench
  subdir('src/bench')
endif

# Examples
if build_examples
  subdir('contrib/examples')
endif

# Fuzz targets
if build_fuzz
  subdir('src/fuzz')
endif

summary({
  'C++ compiler': cxx.get_id(),
  'C++ flags': ' '.join(cxx_flags),
  'C++ linker flags': ' '.join(cxx_lflags),
  'C++ standard': get_option('cpp_std'),
}, section: 'Compiler')

summary({
  'Build benchmarks': build_bench,
  'Build examples': build_examples,
  'Build fuzz targets': build_fuzz,
  'Build tests': build_tests,
  'Library type': lib_type,
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
}, section: 'Configuration')

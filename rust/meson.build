# SPDX-FileCopyrightText: 2026 The Dash Core developers
# SPDX-License-Identifier: MIT

project('rustdeps',
  'rust',
  version: '4.0.0',
  license: 'MIT',
  meson_version: '>=1.9.0',
  default_options: [
    'warning_level=3',
    'rust_std=2018',
  ]
)

# This builds librustdeps - an aggregated library of Rust components for Dash Core

with_grovedb_cxx = get_option('with_grovedb_cxx')

rust_libs = []
rust_lib_names = []
rust_sources = []
rust_compile_args = []

# Conditionally include grovedb_cxx
if with_grovedb_cxx
  message('Building with grovedb_cxx support')
  subdir('grovedb_cxx')
  rust_libs += [grovedb_cxx_lib]
  rust_lib_names += ['grovedb_cxx']
  # Track sources and compile args for aggregated dependency
  rust_sources += [grovedb_cxx_header, grovedb_cxx_source]
  rust_compile_args += ['-I' + meson.current_build_dir() / 'grovedb_cxx' / 'gen']
endif

# Future Rust crates can be added here
# if with_other_crate
#   subdir('other_crate')
#   rust_libs += [other_crate_lib]
#   rust_lib_names += ['other_crate']
#   rust_sources += [other_crate_sources...]
#   rust_compile_args += [...]
# endif

# Create librustdeps.a by aggregating Rust component libraries
if rust_libs.length() > 0
  # Use helper script for proper archive merging
  # This allows Meson to properly expand @INPUT@ and @OUTPUT@
  merge_script = meson.project_source_root() / '..' / 'contrib' / 'meson' / 'merge_archives.sh'

  rustdeps_lib = custom_target('librustdeps',
    output: 'librustdeps.a',
    input: rust_libs,
    command: ['sh', merge_script, '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
  )

  # Declare dependency with aggregated library
  rustdeps_dep = declare_dependency(
    sources: rust_sources,
    compile_args: rust_compile_args,
    link_with: rustdeps_lib,
  )
else
  error('No Rust crates enabled. Enable at least one with -Dwith_<crate>=true')
endif

# Summary
summary({
  'librustdeps includes': rust_lib_names,
  'grovedb_cxx': with_grovedb_cxx,
}, section: 'Rust Components for Dash Core')

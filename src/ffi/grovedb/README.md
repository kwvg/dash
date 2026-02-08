# libgrovedb

C++20 library based on the [`grovedb_cxx`](../../../rust/grovedb_cxx) C++ foreign function interface for a limited subset of [`grovedb`](../../../rust/grovedb)'s public API.

libgrovedb has no API stability guarantees, and will change as required by Dash Core.

## Building

Requires Meson 1.9.0+ or higher. Autotools build files are meant for use with Dash Core only and are not expected to work otherwise.

### Option 1: Link against FFI crate

First, build [`grovedb_cxx`](../../../rust/grovedb_cxx):

```bash
# From root directory
cd rust/grovedb_cxx
meson setup builddir
meson compile -C builddir
```

Then build `libgrovedb`:

```bash
# From this directory
meson setup builddir -Dgrovedb_cxx_build_dir=../../../rust/grovedb_cxx/builddir
meson compile -C builddir
```

### Option 2: Link against Rust dependencies for Dash Core

First, build [`librustdeps`](../../../rust):

```bash
# From root directory
cd rust
meson setup builddir -Dwith_grovedb_cxx=true
meson compile -C builddir
```

Then build `libgrovedb`:

```bash
# From this directory
meson setup builddir -Duse_rustdeps=true
meson compile -C builddir
```

## Build Options

- `-Dbuild_bench=[true|false]`: Build benchmark executable (default: `false`)
- `-Dbuild_fuzz=[true|false]`: Build fuzz targets (default: `false`, requires libFuzzer-capable compiler)
- `-Dbuild_tests=[true|false]`: Build unit tests (default: `true`)
- `-Dgrovedb_cxx_build_dir=<path>`: Path to `grovedb_cxx` build directory (auto-detected if empty, ignored if `use_rustdeps=true`)
- `-Dharden_build=[true|false]`:  Set hardening compiler, linker and preprocessor flags (default: `true`)
- `-Drustdeps_build_dir=<path>`: Path to `librustdeps` build directory (auto-detected if empty)
- `-Dshared_library=[true|false]`: Build shared library with pkg-config definitions (default: `true`, implicit with `build_fuzz` or `build_tests`)
- `-Dsuppress_external_warnings=[true|false]`: Suppress compiler warnings on external sources (default: `true`)
- `-Duse_rustdeps=[true|false]`: Link against `librustdeps` instead of `libgrovedb_cxx` (default: `false`)

## License

libgrovedb is released under the terms of the MIT license. See the accompanying file [LICENSE](./LICENSE) or https://opensource.org/license/mit.

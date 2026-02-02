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
meson setup build
meson compile -C build
```

Then build `libgrovedb`:

```bash
# From this directory
meson setup build -Dgrovedb_cxx_build_dir=../../../rust/grovedb_cxx/build
meson compile -C build
```

### Option 2: Link against Rust dependencies for Dash Core

First, build [`librustdeps`](../../../rust):

```bash
# From root directory
cd rust
meson setup build -Dwith_grovedb_cxx=true
meson compile -C build
```

Then build libgrovedb:

```bash
# From this directory
meson setup build -Duse_rustdeps=true
meson compile -C build
```

### Running tests

```bash
meson setup build -Dbuild_tests=true
meson test -C build
```

## Build Options

- `-Dbuild_tests=true`: Build and run tests (default: `true`)
- `-Duse_rustdeps=true`: Link against librustdeps instead of libgrovedb_cxx directly (default: `false`)
- `-Dgrovedb_cxx_build_dir=<path>`: Path to grovedb_cxx build directory (auto-detected if empty)
- `-Drustdeps_build_dir=<path>`: Path to rustdeps build directory (auto-detected if empty)
- `-Denable-werror=true`: Treat compiler warnings as errors (default: `true`)
- `-Denable-hardening=true`: Enable security hardening flags (default: `true`)
- `-Denable-suppress-external-warnings=true`: Suppress warnings from external headers (default: `true`)
- `-Denable-tsan=true`: Enable ThreadSanitizer (default: `false`)
- `-Denable-ubsan=true`: Enable UndefinedBehaviorSanitizer (default: `false`)
- `-Denable-lcov=true`: Enable code coverage with lcov (default: `false`)
- `-Denable-lcov-branch-coverage=true`: Enable lcov branch coverage (default: `false`)

## License

libgrovedb is released under the terms of the MIT license. See the accompanying file [LICENSE](./LICENSE) or https://opensource.org/license/mit.

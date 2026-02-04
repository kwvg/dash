# Rust dependencies for Dash Core

This directory hosts subtrees to external dependencies and FFI crates for bridging them with Dash Core. For the C++ side of the FFI integration, visit [`src/ffi`](../src/ffi).

The crates provided here have no API stability guarantees and will change as required by Dash Core.

## Building

Requires Meson 1.9.0+ and Rust [1.88](../rust-toolchain.toml). Autotools build files are meant for use with Dash Core only and are not expected to work otherwise.

```bash
# From this directory
meson setup builddir -Dwith_grovedb_cxx=true
meson compile -C builddir
```

### Build options

- `-Dcargo_profile=release`: Build Rust components in release mode (default: `debug`)
- `-Dwith_grovedb_cxx=true`: Include [`grovedb_cxx`](./grovedb_cxx) crate in librustdeps (default: `true`)

## License

This metapackage is subject to the license terms of constituent packages and unless specified otherwise is licensed under the same terms as Dash Core, see project-wide [`COPYING`](../COPYING).

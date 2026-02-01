# Dash Core Nix Build System

This directory contains the Nix-based build infrastructure for Dash Core.

## Philosophy

Unlike the previous Docker-based approach, this Nix system uses the **Guix approach** for building reproducible binaries:

- **Dynamic linker set during build** - We use `-Wl,--dynamic-linker` during configure, NOT patchelf after build
- **depends/ builds libraries** - Qt, Boost, BDB, libevent, etc. are built by depends/, not provided by Nix
- **Nix provides toolchain** - Compilers, build tools, Python testing infrastructure
- **Minimal environments** - Only include what's functionally required

## Architecture

The Nix environments follow an **inclusion hierarchy**:

```
test ⊂ ci ⊂ develop
```

- **test**: Minimal Python testing environment (~13 packages)
- **ci**: test + build tools + target-specific compilers (~29 packages)
- **develop**: ci + all compilers + development tools (~72 packages)

## Directory Structure

```
contrib/nix/
├── README.md              # This file
├── libexec/               # Build scripts
│   ├── env_setup.py      # Environment configuration (dynamic linker setup)
│   ├── build.py          # Main build orchestration
│   └── utils.py          # Helper functions
└── examples/             # Example usage scripts
```

## Usage

### Enter development shell

```bash
# Native macOS development (Apple Silicon)
nix develop .#develop.aarch64-darwin

# Linux development (ARM64)
nix develop .#develop.aarch64-linux

# Linux development (x86-64)
nix develop .#develop.x86_64-linux
```

### Enter CI environment

```bash
# Build for specific target on ARM64 runner
nix develop .#ci.linux64.aarch64-linux
nix develop .#ci.linux64_nowallet.aarch64-linux

# Build for win64 on x86-64 runner (Wine requirement)
nix develop .#ci.win64.x86_64-linux
```

### Build using Python scripts

```bash
# After entering a Nix shell
python3 contrib/nix/libexec/build.py linux64_nowallet

# Or manually
./autogen.sh
make -C depends HOST=x86_64-pc-linux-gnu -j$(nproc)
./configure --prefix=$(pwd)/depends/x86_64-pc-linux-gnu
make -j$(nproc)
```

## Key Differences from Docker Approach

| Aspect | Docker (Old) | Nix (New) |
|--------|--------------|-----------|
| Libraries | apt packages | depends/ builds |
| Binary patching | patchelf after build | `-Wl,--dynamic-linker` during build |
| Package count | 80-100 per environment | 20-27 per environment |
| Environment isolation | Container boundaries | Nix store paths |
| Reproducibility | Docker image tags | Flake locks |

## Why Not Patchelf?

The old approach patched binaries after build:

```bash
# BAD: Patching after build
patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2 ./src/dashd
```

The new Guix-inspired approach sets the correct dynamic linker during configure:

```bash
# GOOD: Set during configure
./configure LDFLAGS="-Wl,--dynamic-linker=/lib64/ld-linux-x86-64.so.2"
```

This means:
- Binaries are built correctly from the start
- No post-build modification needed
- More reproducible and maintainable
- Follows upstream Bitcoin Core Guix approach

## Code Style

All Python scripts in this directory follow these rules (enforced by .editorconfig):

- **Line endings**: LF (Unix)
- **Indentation**: 2 spaces
- **Encoding**: UTF-8
- **Makefiles**: Tabs (exception to 2-space rule)

## Testing

```bash
# Test environment setup script
python3 contrib/nix/libexec/env_setup.py x86_64-pc-linux-gnu

# Test build in Docker (on macOS host)
docker run -it -v $(pwd):/workspace nixos/nix
cd /workspace
nix develop .#ci.linux64_nowallet.aarch64-linux
python3 contrib/nix/libexec/build.py linux64_nowallet

# Verify no Nix paths in binary
readelf -l src/dashd | grep interpreter
# Must show: /lib64/ld-linux-x86-64.so.2 (NOT /nix/store/...)
```

## See Also

- `flake.nix` - Nix flake definition with all environments
- `contrib/guix/libexec/build.sh` - Upstream Guix approach (our inspiration)
- `PLAN.md` - Complete implementation plan

# Nix Environment Validation

This document describes the validation criteria and methodology used to ensure Nix development environments are correctly configured.

## Validation Philosophy

**We validate environment setup, not full builds.**

Building Dash Core and its dependencies is expensive (time, CPU, disk space). Instead, we validate that:
1. The environment provides the correct tools
2. Compilers are the expected versions
3. Environment variables are set correctly
4. All required packages are available

This approach is based on the principle: **"If the right tools are present with the right configuration, the build will work."**

## Validation Criteria

### 1. Flake Syntax Validation
```bash
nix flake check
```
- Must pass without errors
- Validates all devShells evaluate correctly
- Ensures no syntax errors in Nix expressions

### 2. Compiler Verification
For each environment, verify:
- Correct compiler is available (`gcc` or `clang`)
- Correct compiler version matches specification
- Compiler can be invoked without errors

**Expected versions:**
- GCC 11: `gcc (GCC) 11.x.x`
- GCC 15: `gcc (GCC) 15.x.x`
- Clang 19: `clang version 19.x.x`

### 3. Environment Variables
Each CI environment must set:
- `CI_TARGET`: The target name (e.g., `linux64_nowallet`)
- `HOST`: The target triple (e.g., `x86_64-pc-linux-gnu`)
- `CONFIGURE_FLAGS`: Appropriate flags for the build variant

### 4. Build Tools Availability
All CI environments must provide:
- **Build system**: autoconf, automake, cmake, libtool, pkg-config
- **Python**: python3 (3.10.x)
- **Core utilities**: make, bash, tar, gzip, etc.

### 5. Specialized Tools
Environment-specific tools:
- **Sanitizers**: LLVM symbolizer for TSan/UBSan/ASan
- **Fuzzing**: libFuzzer support via Clang
- **Cross-compilation**: Target-specific toolchains
- **Windows**: Wine for running Windows binaries

## Validation Methodology

### Test Environments

We test in **Docker** using `nixos/nix:latest` to ensure:
- Clean, reproducible environment
- No host system dependencies
- Tests work on any machine with Docker

**Platform testing:**
- `aarch64-linux`: Default Docker platform on ARM Macs
- `x86_64-linux`: Using `docker run --platform linux/amd64`

### Test Procedure

For each devShell environment:

```bash
# 1. Flake validation
docker run --rm -v $(pwd):/workspace -w /workspace nixos/nix:latest \
  nix --extra-experimental-features 'nix-command flakes' flake check

# 2. Environment validation
docker run --rm -v $(pwd):/workspace -w /workspace nixos/nix:latest \
  nix --extra-experimental-features 'nix-command flakes' develop .#<env-name> --command bash -c '
    # Check compiler
    gcc --version | head -1  # or clang --version

    # Check environment variables
    echo "CI_TARGET=$CI_TARGET"
    echo "HOST=$HOST"
    echo "CONFIGURE_FLAGS=$CONFIGURE_FLAGS"

    # Check key tools
    which autoconf automake cmake libtool pkg-config python3
  '
```

### What We Don't Test

**We explicitly avoid:**
- Full `depends/` builds (takes 30-60 minutes per target)
- Full Dash Core builds (takes 10-20 minutes)
- Running functional tests (requires full build + dependencies)

**Why:** These are extremely expensive and the CI will catch build failures. Our goal is to validate the **environment setup**, not the **build itself**.

## Validation Results

Validation results are documented in git commit messages:
- Each commit includes "Tested in Docker" section
- Lists what was verified (compiler version, tools, etc.)
- Notes any platform-specific limitations

### Example Validation Output

```
Tested in Docker:
- GCC 15.2.0 verified
- All build tools present (autoconf, automake, cmake, libtool, pkg-config)
- Environment variables configured correctly
- CI_TARGET=linux64_nowallet, HOST=x86_64-pc-linux-gnu
```

## Troubleshooting Failed Validations

### Flake Check Fails
- Check Nix syntax in `flake.nix`
- Ensure all referenced packages exist in nixpkgs
- Verify conditional expressions evaluate correctly

### Compiler Version Wrong
- Check compiler overlay in `flake.nix`
- Verify correct compiler is in buildInputs
- For CI environments using `mkCIEnv`, ensure compiler parameter is correct

### Missing Tools
- Check `buildPackagesList` in `flake.nix`
- Verify packages are available in nixpkgs version
- For cross-compilation, ensure target toolchain is included

### Environment Variables Not Set
- Check `extraShellHook` in environment definition
- Ensure export statements are correct
- Verify no typos in variable names

## Continuous Validation

These environments should be validated:
1. **After each commit** - Verify changes don't break existing environments
2. **Before pushing** - Ensure all environments still work
3. **On nixpkgs updates** - When bumping nixpkgs versions

## Platform-Specific Notes

### x86_64-linux
- Full Wine support for Windows testing
- All environments available (including ci-win64)
- **Testing limitation on ARM Macs**: Docker's x86_64 emulation has seccomp issues with Nix
  - Error: `unable to load seccomp BPF program: Invalid argument`
  - Workaround: Use actual x86_64-linux hardware or CI
  - Alternative: Trust flake check + aarch64-linux validation

### aarch64-linux
- No ci-win64 environment (mingw requires x86)
- All other environments supported
- **Recommended for validation on ARM Macs**: Native platform, no emulation issues
- Test with: `docker run` (no platform flag needed on ARM)

### aarch64-darwin
- Native macOS development
- No cross-compilation FROM macOS (build ON Mac, FOR Mac)
- Test natively without Docker

## Validation Results Summary

### What We Validated

**Platform: aarch64-linux (ARM) - FULLY VALIDATED ✓**

All environments tested in Docker on ARM Mac:
- ✓ `test` - Python 3.10.16, all linters
- ✓ `ci-linux64-nowallet` - GCC 15.2.0
- ✓ `ci-linux64` - GCC 15.2.0
- ✓ `ci-linux64-fuzz` - Clang 19.1.7 + llvm-symbolizer
- ✓ `ci-linux64-tsan` - Clang 19.1.7
- ✓ `ci-linux64-ubsan` - Clang 19.1.7
- ✓ `ci-linux64-sqlite` - GCC 15.2.0
- ✓ `ci-linux64-multiprocess` - Clang 19.1.7
- ✓ `ci-arm-linux` - GCC 11 + ARM cross-toolchain
- ✓ `ci-mac` - Clang 19.1.7 + LLD

**Platform: x86_64-linux - SYNTAX VALIDATED ✓**

Validation status:
- ✓ Flake syntax check passes for all environments
- ✓ All devShells evaluate correctly
- ✓ `ci-win64` correctly excluded on non-x86 platforms
- ⚠️  Runtime validation blocked by Docker emulation limitations
- ✓ Will be validated in CI on actual x86_64-linux hardware

**Platform: aarch64-darwin - NOT YET IMPLEMENTED**
- Develop environments planned for Phase 4

## Docker Emulation Limitations

When running `docker run --platform linux/amd64` on ARM Macs:
- QEMU emulates x86_64 instruction set
- Nix's seccomp sandbox fails with: `unable to load seccomp BPF program: Invalid argument`
- This is a known limitation of QEMU + seccomp + Nix combination
- Not a bug in our flake - affects all Nix operations under x86_64 emulation on ARM

**Workarounds:**
1. Use actual x86_64-linux hardware for testing
2. Use GitHub Actions CI (runs on native x86_64-linux)
3. Trust aarch64-linux validation + flake check (recommended)

**Why trust aarch64-linux validation?**
- Same Nix code, same package versions
- Compilers are architecture-independent (same source)
- Only difference is the host architecture
- If it works on ARM, it will work on x86_64

## Future Enhancements

Potential future validation improvements:
- Automated validation script
- Pre-commit hooks to run validations
- CI job that validates all environments
- Cache validation results to speed up checks

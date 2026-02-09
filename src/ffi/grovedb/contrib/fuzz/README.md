# Fuzzing helpers for macOS

Tested on macOS Tahoe (ARM64 / Apple Silicon only).

## Prerequisites

- [Nix](https://nixos.org/download/) with flakes enabled
- macOS on Apple Silicon (Intel Macs are not supported)

## Quick start

### 1. Enter the development environment

From the `src/ffi/grovedb/` directory:

```bash
nix develop
```

This provides LLVM 20, AFL++ (pre-built Homebrew bottle fetched automatically),
Meson, Ninja, rsync, and Python 3.

### 2. Build the Rust library (one-time)

If you haven't built the Rust/CXX bridge yet:

```bash
cd ../../../rust   # from src/ffi/grovedb/ → repo root rust/
meson setup builddir -Dwith_grovedb_cxx=true
meson compile -C builddir
cd -
```

### 3. Build fuzz targets with AFL++ instrumentation

```bash
CC=afl-cc CXX=afl-c++ meson setup /tmp/fuzz_afl_builddir \
  -Dbuild_fuzz=true -Dbuild_tests=false -Duse_rustdeps=true
meson compile -C /tmp/fuzz_afl_builddir
```

### 4. Run a fuzzing campaign

```bash
python3 contrib/fuzz/campaign.py \
  --builddir /tmp/fuzz_afl_builddir \
  --output /path/to/persistent/results \
  --threads 8
```

The coordinator will:
- Discover all `fuzz_*` harness binaries in the build directory
- Budget concurrent instances based on available threads (4 per harness)
- Create a RAM disk per harness to avoid SSD wear
- Launch a worker process per harness
- Periodically flush results to the persistent output directory
- On SIGINT/SIGTERM, cleanly stop workers, flush, and destroy RAM disks

## Tools

All three tools (`ramdisk.py`, `worker.py`, `campaign.py`) are self-contained
and can be used independently or composed in custom scripts.

### `ramdisk.py`

Standalone RAM disk manager using macOS `hdiutil`/`diskutil`.

```bash
# Create a 2 GB RAM disk named "my_fuzz"
python3 contrib/fuzz/ramdisk.py create --size 2048 --name my_fuzz

# Destroy it
python3 contrib/fuzz/ramdisk.py destroy --name my_fuzz
```

### `worker.py`

Standalone AFL++ worker for a single fuzz harness.  Requires the scratch
directory to be on a RAM disk (refuses to start otherwise).

```bash
# Create a RAM disk first
python3 contrib/fuzz/ramdisk.py create --size 2048 --name my_fuzz

# Run a single harness
python3 contrib/fuzz/worker.py \
  --harness /tmp/fuzz_afl_builddir/src/fuzz/fuzz_batch \
  --scratch /Volumes/my_fuzz \
  --corpus /path/to/seeds/batch

# Clean up
python3 contrib/fuzz/ramdisk.py destroy --name my_fuzz
```

| Flag | Default | Description |
|------|---------|-------------|
| `--harness` | (required) | Path to AFL++-instrumented fuzz harness |
| `--scratch` | (required) | Scratch directory (must be on a RAM disk) |
| `--corpus` | none | Seed corpus directory for this target |
| `--timeout` | 5000 | Per-execution timeout in milliseconds |

The worker:
- Validates the scratch directory is backed by a RAM disk
- Creates `input/` and `output/` directories under scratch
- Seeds the corpus from `--corpus` or generates a minimal seed
- Launches `afl-fuzz` with macOS-appropriate environment variables
- Forwards SIGTERM/SIGINT to `afl-fuzz` for clean shutdown

### `campaign.py`

Coordinator that discovers all harnesses and runs them using `ramdisk.py` and
`worker.py`.  This is the recommended entry point for fuzzing campaigns.

| Flag | Default | Description |
|------|---------|-------------|
| `--builddir` | (required) | Meson build directory |
| `--output` | (required) | Persistent directory for flushed results |
| `--threads` | (required) | Total CPU threads to allocate |
| `--ramdisk-size` | 2048 | RAM disk size in MB per harness |
| `--flush-interval` | 600 | Seconds between flushes to persistent storage |
| `--corpus` | none | Seed corpus directory (subdirs named by target) |
| `--timeout` | 5000 | Per-execution timeout in milliseconds |

#### Resource budgeting

Each harness slot requires a minimum of 4 threads and one RAM disk
(default 2 GB). If the thread budget cannot cover all harnesses simultaneously,
the remaining harnesses are queued and started as slots free up.

#### Flush and persistence

AFL++ output lives on RAM disks during fuzzing. A background thread copies
results to the `--output` directory every `--flush-interval` seconds using
`rsync`. A final flush runs during shutdown.

#### Seed corpus

If `--corpus` is provided, the coordinator looks for a subdirectory matching
each target name (e.g., `--corpus /path/to/seeds` and target `fuzz_wire` uses
`/path/to/seeds/wire/`). If no matching subdirectory exists, a minimal seed is
generated automatically.

## Linting

Check Nix file formatting:

```bash
python3 contrib/lint-nix.py
```

This uses the formatter configured in `flake.nix` (`nixfmt`).  Run from inside
the devenv to ensure `nix fmt` is available.

## Intel Macs

Not supported. `nix develop` will fail with an attribute-not-found error on
`x86_64-darwin` because only `aarch64-darwin` is listed in the flake.

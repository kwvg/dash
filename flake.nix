{
  description = "Dash Core build environments";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    nixpkgs-unstable.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, nixpkgs-unstable }:
    let
      # Supported systems
      systems = [ "aarch64-linux" "x86_64-linux" "aarch64-darwin" ];

      # Helper to generate attributes for each system
      forAllSystems = nixpkgs.lib.genAttrs systems;

      # Compiler overlay - pins specific compiler versions
      # NO patchelf needed - binaries built with correct --dynamic-linker
      compilerOverlay = final: prev:
        let
          unstable = import nixpkgs-unstable {
            inherit (final) system;
            config.allowUnfree = false;
          };
        in {
          # GCC versions
          gcc11 = prev.gcc11;          # From stable (oldest supported for depends)
          gcc14 = prev.gcc14 or unstable.gcc14;  # Fallback to unstable if not in stable
          gcc15 = unstable.gcc15;      # Latest from unstable

          # Clang/LLVM 19 (for sanitizers, multiprocess, fuzz)
          clang_19 = unstable.clang_19;
          llvm_19 = unstable.llvm_19;
          libcxx_19 = unstable.libcxx_19;
          libcxxabi_19 = unstable.libcxxabi_19;
          clang-tools_19 = unstable.clang-tools_19;  # For clang-tidy

          # LLD linker (for macOS cross-compilation)
          lld_19 = unstable.lld_19;
        };

      # Helper to get pkgs for a system with compiler overlay
      pkgsFor = system: import nixpkgs {
        inherit system;
        overlays = [ compilerOverlay ];
        config = {
          allowUnfree = false;
        };
      };

      # Helper to get unstable pkgs for a system
      pkgsUnstableFor = system: import nixpkgs-unstable {
        inherit system;
        config = {
          allowUnfree = false;
        };
      };

      # Build packages for CI environments
      # Matches contrib/containers/ci/ci.Dockerfile
      buildPackagesList = pkgs: with pkgs; [
        autoconf
        automake
        bc
        bear
        bison
        ccache
        cmake
        file
        gawk
        gettext
        gmp
        gmpxx
        libtool
        m4
        parallel
        pkg-config
        python310  # For build scripts
        unzip
        which
        zip
      ];

      # Cross-compilation toolchains
      crossToolchains = pkgs: with pkgs; [
        # ARM Linux (for ci-arm-linux)
        pkgsCross.armv7l-hf-multiplatform.stdenv.cc

        # Windows (for ci-win64)
        pkgsCross.mingwW64.stdenv.cc
        wine
      ];

    in {
      # Development shells will be added in later commits
      # Structure will be:
      #   devShells.<system>.test.<variant>
      #   devShells.<system>.ci.<target>.<host>
      #   devShells.<system>.develop.<variant>
      #
      # Using contrib/nix/libexec/build.py instead of patchelf
      # Binaries built with correct --dynamic-linker from the start

      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
          python = pkgs.python310;  # 3.10.x from nixpkgs-24.11

          # Import pinned package hashes
          pythonHashes = import ./contrib/nix/python-hashes.nix;

          # Helper: Get package (pinned version or nixpkgs fallback)
          getPythonPkg = name: info:
            if info == {} || !(info ? version) then
              python.pkgs.${name}  # Use nixpkgs version
            else
              python.pkgs.buildPythonPackage {
                pname = name;
                version = info.version;
                format = "setuptools";
                src = pkgs.fetchurl {
                  url = if info ? url then info.url
                        else "https://files.pythonhosted.org/packages/source/${builtins.substring 0 1 name}/${name}/${name}-${info.version}.tar.gz";
                  inherit (info) hash;
                };
                doCheck = false;
                nativeBuildInputs = with python.pkgs; [ setuptools ];
                propagatedBuildInputs = with python.pkgs; [
                ] ++ (if name == "mypy" then [ typing-extensions mypy-extensions ] else [])
                  ++ (if name == "flake8" then [ pyflakes pycodestyle mccabe ] else []);
              };

          # Layer 1: Test environment (minimal - Python + linters only)
          # Matches contrib/containers/ci/ci-slim.Dockerfile
          testEnv = pkgs.mkShell {
            name = "dash-test-env";

            buildInputs = [
              # Python 3.10 with test packages
              (python.withPackages (ps: [
                (getPythonPkg "pyzmq" pythonHashes.pyzmq)
                (getPythonPkg "jinja2" pythonHashes.jinja2)
                (getPythonPkg "dash_hash" pythonHashes.dash_hash)
                (getPythonPkg "multiprocess" pythonHashes.multiprocess)
                # lief not needed for tests, only for CI builds
              ]))

              # Linters - pinned versions
              (getPythonPkg "codespell" pythonHashes.codespell)
              (getPythonPkg "flake8" pythonHashes.flake8)
              (getPythonPkg "mypy" pythonHashes.mypy)
              (getPythonPkg "vulture" pythonHashes.vulture)

              # Static analysis tools
              pkgs.cppcheck    # 2.16.0 in nixpkgs (close to 2.13.0)
              pkgs.shellcheck  # 0.10.0 in nixpkgs (vs 0.8.0 in Docker)
            ];

            shellHook = ''
              echo "Dash Core Test Environment (Layer 1)"
              echo "Python $(python3 --version | cut -d' ' -f2) + linters - NO build tools"
              echo ""
              echo "Package versions:"
              echo "  codespell==${pythonHashes.codespell.version} (pinned)"
              echo "  flake8==${pythonHashes.flake8.version} (pinned)"
              echo "  mypy==${pythonHashes.mypy.version} (pinned)"
              echo "  vulture==${pythonHashes.vulture.version} (pinned)"
              echo "  dash_hash==${pythonHashes.dash_hash.version} (pinned)"
              echo "  pyzmq=nixpkgs (Docker: 24.0.1, source doesn't compile)"
              echo "  jinja2=nixpkgs (not pinned in Docker)"
              echo "  multiprocess=nixpkgs (not pinned in Docker)"
            '';
          };

          # Layer 2: CI environment builder
          # Adds build tools + compilers to test environment
          # Matches contrib/containers/ci/ci.Dockerfile
          mkCIEnv = { name, compiler, extraBuildInputs ? [], extraShellHook ? "" }: pkgs.mkShellNoCC {
            inherit name;

            # Use mkShellNoCC to avoid default stdenv compiler (GCC 13)
            # Then explicitly add our desired compiler
            buildInputs = [ compiler ]
              ++ (buildPackagesList pkgs)
              ++ [
                # Python 3.10 with test packages (from testEnv)
                (python.withPackages (ps: [
                  (getPythonPkg "pyzmq" pythonHashes.pyzmq)
                  (getPythonPkg "jinja2" pythonHashes.jinja2)
                  (getPythonPkg "dash_hash" pythonHashes.dash_hash)
                  (getPythonPkg "multiprocess" pythonHashes.multiprocess)
                ]))
                # Linters (from testEnv)
                (getPythonPkg "codespell" pythonHashes.codespell)
                (getPythonPkg "flake8" pythonHashes.flake8)
                (getPythonPkg "mypy" pythonHashes.mypy)
                (getPythonPkg "vulture" pythonHashes.vulture)
                # Static analysis (from testEnv)
                pkgs.cppcheck
                pkgs.shellcheck
              ]
              ++ extraBuildInputs;

            shellHook = ''
              ${testEnv.shellHook}
              echo ""
              echo "=== CI Environment: ${name} ==="
              echo "Compiler: ${compiler.name}"
              echo "Build tools available: autoconf, automake, cmake, libtool, pkg-config"
              echo ""
              echo "Build scripts:"
              echo "  contrib/nix/libexec/env_setup.py - Environment configuration"
              echo "  contrib/nix/libexec/build.py - Build orchestration"
              echo ""
              ${extraShellHook}
            '';
          };

        in {
          # Default shell
          default = pkgs.mkShell {
            name = "dash-core-default";
            buildInputs = [ ];
            shellHook = ''
              echo "Dash Core Nix Development Environment"
              echo "See contrib/nix/README.md for usage"
              echo ""
              echo "Available environments:"
              echo "  nix develop .#test - Test environment (Python + linters)"
              echo "  nix develop .#ci-linux64-nowallet - CI: GCC 15, no wallet"
              echo "  nix develop .#ci-linux64 - CI: GCC 15, full build"
              echo "  nix develop .#ci-linux64-fuzz - CI: Clang 19, fuzzing"
              echo "  nix develop .#ci-linux64-tsan - CI: Clang 19, TSan"
              echo "  nix develop .#ci-linux64-ubsan - CI: Clang 19, UBSan"
              echo "  nix develop .#ci-linux64-sqlite - CI: GCC 15, SQLite"
            '';
          };

          # Test environment (Layer 1)
          test = testEnv;

          # CI environments (Layer 2)
          # Each environment builds on test + adds compilers and build tools

          # linux64_nowallet - GCC 15, no wallet, no GUI
          # Matches CI_TARGET=linux64_nowallet in ci.Dockerfile
          ci-linux64-nowallet = mkCIEnv {
            name = "dash-ci-linux64-nowallet";
            compiler = pkgs.gcc15;
            extraShellHook = ''
              export CI_TARGET="linux64_nowallet"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--disable-wallet --without-gui --without-bdb --without-sqlite --enable-reduce-exports"
              export MAKE_FLAGS="NO_WALLET=1"
              echo "CI Target: linux64_nowallet"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: GCC 15"
              echo "  Features: NO wallet, NO GUI"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST \$MAKE_FLAGS -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };

          # linux64 - GCC 15, full build with wallet and GUI
          # Matches CI_TARGET=linux64 in ci.Dockerfile
          ci-linux64 = mkCIEnv {
            name = "dash-ci-linux64";
            compiler = pkgs.gcc15;
            extraShellHook = ''
              export CI_TARGET="linux64"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process"
              echo "CI Target: linux64"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: GCC 15"
              echo "  Features: Wallet (BDB + SQLite), GUI"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };

          # linux64_fuzz - Clang 19, fuzzing with libFuzzer
          # Matches CI_TARGET=linux64_fuzz in ci.Dockerfile
          ci-linux64-fuzz = mkCIEnv {
            name = "dash-ci-linux64-fuzz";
            compiler = pkgs.clang_19;
            extraBuildInputs = [ pkgs.llvm_19 ];
            extraShellHook = ''
              export CI_TARGET="linux64_fuzz"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--enable-fuzz --with-sanitizers=fuzzer,address,undefined --disable-wallet --without-gui --without-bdb --without-sqlite CC=clang CXX=clang++"
              export MAKE_FLAGS="NO_WALLET=1"
              echo "CI Target: linux64_fuzz"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: Clang 19"
              echo "  Features: Fuzz testing (libFuzzer + ASan + UBSan)"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST \$MAKE_FLAGS -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };

          # linux64_tsan - Clang 19 with ThreadSanitizer
          # Matches CI_TARGET=linux64_tsan in ci.Dockerfile
          ci-linux64-tsan = mkCIEnv {
            name = "dash-ci-linux64-tsan";
            compiler = pkgs.clang_19;
            extraBuildInputs = [ pkgs.llvm_19 ];
            extraShellHook = ''
              export CI_TARGET="linux64_tsan"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--enable-reduce-exports --with-sanitizers=thread --with-boost-process CC=clang CXX=clang++"
              echo "CI Target: linux64_tsan"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: Clang 19"
              echo "  Features: ThreadSanitizer (TSan) for race detection"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };

          # linux64_ubsan - Clang 19 with UndefinedBehaviorSanitizer
          # Matches CI_TARGET=linux64_ubsan in ci.Dockerfile
          ci-linux64-ubsan = mkCIEnv {
            name = "dash-ci-linux64-ubsan";
            compiler = pkgs.clang_19;
            extraBuildInputs = [ pkgs.llvm_19 ];
            extraShellHook = ''
              export CI_TARGET="linux64_ubsan"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--enable-reduce-exports --with-sanitizers=undefined --with-boost-process CC=clang CXX=clang++"
              echo "CI Target: linux64_ubsan"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: Clang 19"
              echo "  Features: UndefinedBehaviorSanitizer (UBSan)"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };

          # linux64_sqlite - GCC 15 with SQLite wallet (no BDB)
          # Matches CI_TARGET=linux64_sqlite in ci.Dockerfile
          ci-linux64-sqlite = mkCIEnv {
            name = "dash-ci-linux64-sqlite";
            compiler = pkgs.gcc15;
            extraShellHook = ''
              export CI_TARGET="linux64_sqlite"
              export HOST="x86_64-pc-linux-gnu"
              export CONFIGURE_FLAGS="--enable-reduce-exports --with-boost-process --with-sqlite --without-bdb"
              echo "CI Target: linux64_sqlite"
              echo "  Host: x86_64-pc-linux-gnu"
              echo "  Compiler: GCC 15"
              echo "  Features: SQLite wallet only (no BDB)"
              echo ""
              echo "Build commands:"
              echo "  ./autogen.sh"
              echo "  make -C depends HOST=\$HOST -j\$(nproc)"
              echo "  ./configure --prefix=\$(pwd)/depends/\$HOST \$CONFIGURE_FLAGS"
              echo "  make -j\$(nproc)"
            '';
          };
        });
    };
}

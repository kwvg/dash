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
          compiler-rt_19 = unstable.compiler-rt_19;
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
              echo "  nix develop .#test - Test environment (Python ${pythonHashes.codespell.version} + linters)"
            '';
          };

          # Test environment (Layer 1)
          test = testEnv;
        });
    };
}

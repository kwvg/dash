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
      compilerOverlay = import ./contrib/nix/common/compilers.nix {
        inherit nixpkgs nixpkgs-unstable;
      };

      # Helper to get pkgs for a system with compiler overlay
      pkgsFor = system: import nixpkgs {
        inherit system;
        overlays = [ compilerOverlay ];
        config = {
          allowUnfree = false;
        };
      };

    in {
      devShells = forAllSystems (system:
        let
          pkgs = pkgsFor system;
          python = pkgs.python310;  # 3.10.x from nixpkgs-24.11

          # Import pinned package hashes
          pythonHashes = import ./contrib/nix/python-hashes.nix;

          # Import package lists
          packageLists = import ./contrib/nix/common/packages.nix { inherit pkgs; };

          # Import helper functions
          helpers = import ./contrib/nix/common/helpers.nix {
            inherit pkgs python pythonHashes packageLists;
          };

          # Test environment
          testEnv = import ./contrib/nix/test/default.nix {
            inherit pkgs python pythonHashes helpers;
          };

          # CI environments
          ciEnvs = import ./contrib/nix/ci/default.nix {
            inherit system pkgs helpers;
            testEnvShellHook = testEnv.shellHook;
          };

          # Develop environment
          developEnv = import ./contrib/nix/develop/default.nix {
            inherit system helpers;
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
              echo "  nix develop .#ci_linux-x86_64_nowallet - CI: GCC 13, no wallet"
              echo "  nix develop .#ci_linux-x86_64 - CI: GCC 13, full build"
              echo "  nix develop .#ci_linux-x86_64_fuzz - CI: Clang 19, fuzzing"
              echo "  nix develop .#ci_linux-x86_64_tsan - CI: Clang 19, TSan"
              echo "  nix develop .#ci_linux-x86_64_ubsan - CI: Clang 19, UBSan"
              echo "  nix develop .#ci_linux-x86_64_sqlite - CI: GCC 13, SQLite"
              echo "  nix develop .#ci_linux-x86_64_multiprocess - CI: Clang 19, multiprocess"
              echo "  nix develop .#ci_linux-aarch64 - CI: GCC 13, ARM64 cross"
              echo "  nix develop .#ci_darwin-x86_64 - CI: Clang 19, macOS cross"
              ${if system == "x86_64-linux" then ''echo "  nix develop .#ci_mingw64-x86_64 - CI: GCC 13, Windows cross + Wine"'' else ""}
              echo ""
              echo "Development environments (all tools):"
              echo "  nix develop .#develop - All compilers, all tools"
            '';
          };

          # Test environment (Layer 1)
          test = testEnv;

          # Develop environment (Layer 3)
          develop = developEnv;

          # CI environments (Layer 2)
          ci_linux-x86_64_nowallet = ciEnvs.ci_linux-x86_64_nowallet;
          ci_linux-x86_64 = ciEnvs.ci_linux-x86_64;
          ci_linux-x86_64_fuzz = ciEnvs.ci_linux-x86_64_fuzz;
          ci_linux-x86_64_tsan = ciEnvs.ci_linux-x86_64_tsan;
          ci_linux-x86_64_ubsan = ciEnvs.ci_linux-x86_64_ubsan;
          ci_linux-x86_64_sqlite = ciEnvs.ci_linux-x86_64_sqlite;
          ci_linux-x86_64_multiprocess = ciEnvs.ci_linux-x86_64_multiprocess;
          ci_linux-aarch64 = ciEnvs.ci_linux-aarch64;
          ci_darwin-x86_64 = ciEnvs.ci_darwin-x86_64;
        } // (if system == "x86_64-linux" then {
          # mingw64 only on x86_64-linux (mingw requires x86)
          ci_mingw64-x86_64 = ciEnvs.ci_mingw64-x86_64;
        } else {}));
    };
}

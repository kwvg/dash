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

      devShells = forAllSystems (system: {
        # Placeholder - will be populated in subsequent commits
        default = (pkgsFor system).mkShell {
          name = "dash-core-default";
          buildInputs = [ ];
          shellHook = ''
            echo "Dash Core Nix Development Environment"
            echo "See contrib/nix/README.md for usage"
            echo ""
            echo "Available environments will be added in subsequent commits"
          '';
        };
      });
    };
}

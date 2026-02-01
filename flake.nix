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

      # Helper to get pkgs for a system
      pkgsFor = system: import nixpkgs {
        inherit system;
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

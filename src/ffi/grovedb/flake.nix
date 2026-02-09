{
  description = "GroveDB FFI fuzzing environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
    }:
    let
      systems = [
        "aarch64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      pkgsFor = system: nixpkgs.legacyPackages.${system};
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
          aflplusplus = pkgs.callPackage ./contrib/fuzz/hb_aflplusplus.nix { };
        in
        {
          default = import ./contrib/fuzz/devenv.nix { inherit pkgs aflplusplus; };
        }
      );

      formatter = forAllSystems (system: (pkgsFor system).nixfmt);
    };
}

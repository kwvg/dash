# CI Environments (Layer 2)
# Each environment builds on test + adds compilers and build tools
{ system, pkgs, helpers, testEnvShellHook }:

let
  # Common arguments for all CI environments
  ciArgs = {
    inherit pkgs helpers testEnvShellHook;
  };
in
{
  # Linux x86_64 environments
  ci-linux64-nowallet = import ./linux64-nowallet.nix ciArgs;
  ci-linux64 = import ./linux64.nix ciArgs;
  ci-linux64-fuzz = import ./linux64-fuzz.nix ciArgs;
  ci-linux64-tsan = import ./linux64-tsan.nix ciArgs;
  ci-linux64-ubsan = import ./linux64-ubsan.nix ciArgs;
  ci-linux64-sqlite = import ./linux64-sqlite.nix ciArgs;
  ci-linux64-multiprocess = import ./linux64-multiprocess.nix ciArgs;

  # Cross-compilation environments
  ci-arm-linux = import ./arm-linux.nix ciArgs;
  ci-mac = import ./mac.nix ciArgs;

  # Windows cross-compilation (x86_64-linux only)
  # Conditionally included in main flake.nix
  ci-win64 = import ./win64.nix ciArgs;
}

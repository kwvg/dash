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
  ci_linux-x86_64_nowallet = import ./linux-x86_64_nowallet.nix ciArgs;
  ci_linux-x86_64 = import ./linux-x86_64.nix ciArgs;
  ci_linux-x86_64_fuzz = import ./linux-x86_64_fuzz.nix ciArgs;
  ci_linux-x86_64_tsan = import ./linux-x86_64_tsan.nix ciArgs;
  ci_linux-x86_64_ubsan = import ./linux-x86_64_ubsan.nix ciArgs;
  ci_linux-x86_64_sqlite = import ./linux-x86_64_sqlite.nix ciArgs;
  ci_linux-x86_64_multiprocess = import ./linux-x86_64_multiprocess.nix ciArgs;

  # Cross-compilation environments
  ci_linux-aarch64 = import ./linux-aarch64.nix ciArgs;
  ci_darwin-x86_64 = import ./darwin-x86_64.nix ciArgs;

  # Windows cross-compilation (x86_64-linux only)
  # Conditionally included in main flake.nix
  ci_mingw64-x86_64 = import ./mingw64-x86_64.nix ciArgs;
}

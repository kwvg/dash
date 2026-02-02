# Package lists for different environment types
{ pkgs }:

{
  # Build packages for CI environments
  # Matches contrib/containers/ci/ci.Dockerfile
  buildPackages = with pkgs; [
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

  # Development tools (for develop environments)
  devTools = with pkgs; [
    # Clang tools for code quality
    llvmPackages_19.clang-tools  # clang-tidy, clang-format, etc.

    # Build system generators
    bear  # Generate compile_commands.json

    # Code analysis
    cppcheck
    shellcheck

    # Performance tools
    ccache

    # Documentation
    doxygen

    # Version control
    git
  ];
}

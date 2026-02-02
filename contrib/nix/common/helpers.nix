# Helper functions for creating development environments
{ pkgs, python, pythonHashes, packageLists }:

rec {
  # Helper: Get pinned Python package or fall back to nixpkgs
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

  # Layer 1: Test environment packages
  testPackages = [
    # Python 3.10 with test packages
    (python.withPackages (ps: [
      (getPythonPkg "pyzmq" pythonHashes.pyzmq)
      (getPythonPkg "jinja2" pythonHashes.jinja2)
      (getPythonPkg "dash_hash" pythonHashes.dash_hash)
      (getPythonPkg "multiprocess" pythonHashes.multiprocess)
    ]))

    # Linters - pinned versions
    (getPythonPkg "codespell" pythonHashes.codespell)
    (getPythonPkg "flake8" pythonHashes.flake8)
    (getPythonPkg "mypy" pythonHashes.mypy)
    (getPythonPkg "vulture" pythonHashes.vulture)

    # Static analysis tools
    pkgs.cppcheck
    pkgs.shellcheck
  ];

  # Layer 2: CI environment builder
  # Adds build tools + compilers to test environment
  mkCIEnv = { name, compiler, extraBuildInputs ? [], extraShellHook ? "", testEnvShellHook }:
    pkgs.mkShellNoCC {
      inherit name;

      # Use mkShellNoCC to avoid default stdenv compiler (GCC 13)
      # Then explicitly add our desired compiler
      buildInputs = [ compiler ]
        ++ packageLists.buildPackages
        ++ testPackages
        ++ extraBuildInputs;

      shellHook = ''
        ${testEnvShellHook}
        echo ""
        echo "=== CI Environment: ${name} ==="
        echo "Compiler: ${compiler.name}"
        echo "Build tools available: autoconf, automake, cmake, libtool, pkg-config"
        echo ""
        echo "Build scripts:"
        echo "  contrib/nix/scripts/env_setup.py - Environment configuration"
        echo "  contrib/nix/scripts/build.py - Build orchestration"
        echo ""
        ${extraShellHook}
      '';
    };

  # Layer 3: Develop environment builder
  # Kitchen sink - all compilers, all tools, everything for local development
  mkDevelopEnv = { name, system }: pkgs.mkShellNoCC {
    inherit name;

    # All compilers available
    buildInputs = [
      # GCC 15 as default
      pkgs.gcc15

      # Clang/LLVM 19 suite
      pkgs.clang_19
      pkgs.llvm_19
      pkgs.lld_19
    ]
    ++ packageLists.buildPackages
    ++ packageLists.devTools
    ++ testPackages
    ++ (if system == "x86_64-linux" then [ pkgs.wine ] else []);

    shellHook = ''
      echo "Dash Core Development Environment (Layer 3)"
      echo "Full-featured environment for local development"
      echo ""
      echo "=== Available Compilers ==="
      echo "  GCC 15: $(gcc --version 2>&1 | head -1) [default]"
      echo "  Clang 19: $(clang --version 2>&1 | head -1)"
      echo ""
      echo "=== Development Tools ==="
      echo "  Code quality: clang-tidy, clang-format, cppcheck"
      echo "  Build tools: bear (compile_commands.json), ccache"
      echo "  Analysis: shellcheck, mypy, flake8, vulture, codespell"
      echo "  Documentation: doxygen"
      ${if system == "x86_64-linux" then ''echo "  Windows: wine"'' else ""}
      echo ""
      echo "=== Quick Start ==="
      echo "  1. Build depends:"
      echo "     make -C depends HOST=x86_64-pc-linux-gnu -j\$(nproc)"
      echo ""
      echo "  2. Configure (GCC 15):"
      echo "     ./autogen.sh"
      echo "     ./configure --prefix=\$(pwd)/depends/x86_64-pc-linux-gnu"
      echo ""
      echo "  3. Build Dash Core:"
      echo "     make -j\$(nproc)"
      echo ""
      echo "  4. Run tests:"
      echo "     ./src/test/test_dash"
      echo "     test/functional/test_runner.py"
      echo ""
      echo "=== Alternative Compilers ==="
      echo "  With Clang 19:"
      echo "    CC=clang CXX=clang++ ./configure ..."
      echo ""
      echo "=== Code Quality ==="
      echo "  Linting:"
      echo "    test/lint/all-lint.py"
      echo ""
      echo "  Generate compile_commands.json:"
      echo "    bear -- make -j\$(nproc)"
      echo ""
      echo "  Run clang-tidy:"
      echo "    clang-tidy -p . src/*.cpp"
      echo ""
    '';
  };
}

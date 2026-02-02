# Test environment (Layer 1)
# Minimal environment with Python 3.10 and linters only
# Matches contrib/containers/ci/ci-slim.Dockerfile
{
  pkgs,
  python,
  pythonHashes,
  helpers,
}:

pkgs.mkShell {
  name = "dash-test-env";

  buildInputs = helpers.testPackages;

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
}

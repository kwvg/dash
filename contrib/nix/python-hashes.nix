# Python package versions for Dash Core Nix environments
#
# NOTE: Nix builds from SOURCE, Docker uses BINARY WHEELS from PyPI.
# Some packages (pyzmq, lief) don't compile from source with modern GCC,
# so we use nixpkgs versions instead of exact Docker versions.
#
# Pinned packages (exact versions):
# - codespell==2.1.0 ✓
# - flake8==4.0.1 ✓
# - mypy==0.981 ✓
# - vulture==2.6 ✓
#
# Nixpkgs versions (compilation issues with pinned versions):
# - pyzmq (Docker: 24.0.1, Nix: ~26.x from nixpkgs)
# - lief (Docker: 0.13.2, Nix: ~0.14.x from nixpkgs)
# - dash_hash (Docker: 1.4.0, Nix: 1.4.0 ✓)
#
# Unpinned in Docker:
# - jinja2, multiprocess
{
  # Successfully pinned packages
  codespell = {
    version = "2.1.0";
    hash = "sha256-GdP+VkT+80JXd+ZvIlqMgtOQWdz+ntszSaiiz0g4PuU=";
  };

  flake8 = {
    version = "4.0.1";
    hash = "sha256-gG4DTdpEEUgV4jwW75L5XJHkxxEA/1KBOt9xMqathw0=";
  };

  mypy = {
    version = "0.981";
    hash = "sha256-rXfBMDfTQC++/9oH1R4/IougeNHHCWpzdZyUGeoDG/Q=";
  };

  vulture = {
    version = "2.6";
    hash = "sha256-JRX6hIGBAB3Ipzq6agGhoXQG9dNy8k7H9xkYZvn0mX4=";
  };

  # GitHub packages
  dash_hash = {
    version = "1.4.0";
    hash = "sha256-JJD+sLsYagOVahQLsHfRlvpOKwwixd8FF8Xxu9bMflo=";
    url = "https://github.com/dashpay/dash_hash/archive/1.4.0.tar.gz";
  };

  # Use nixpkgs versions (source doesn't compile with modern GCC)
  pyzmq = {};  # nixpkgs has ~26.x
  lief = {};   # nixpkgs has ~0.14.x

  # Unpinned in Docker
  jinja2 = {};
  multiprocess = {};
}

# Develop environment (Layer 3)
# Full-featured environment for local development
# Includes all compilers, all tools, everything
{ system, helpers }:

helpers.mkDevelopEnv {
  name = "dash-develop";
  inherit system;
}

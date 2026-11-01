{
  description = "Matrix rain reproducible ESP-IDF development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
  };

  outputs = {
    nixpkgs,
    nixpkgs-esp-dev,
    ...
  }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          espShell = nixpkgs-esp-dev.devShells.${system}.esp-idf-full;
        in {
          default = pkgs.mkShell {
            inputsFrom = [ espShell ];
            packages = with pkgs; [
              cmake
              ninja
              git
              jq
            ];
          };
        });
    };
}

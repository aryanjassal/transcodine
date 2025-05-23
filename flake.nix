{
  description = "Transcodine devshell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells.${system}.default = pkgs.mkShell {
        name = "transcodine-devshell";
        nativeBuildInputs = with pkgs; [ clang-tools gcc gdb gnumake valgrind ];
        shellHook = ''
          PATH=./build:$PATH
        '';
      };
    };
}

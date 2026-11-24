{
  description = "A basic flake with a shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            libGLU
            raylib
            raygui

            # X11 dependencies
            libX11
            libX11.dev
            libXcursor
            libXi
            libXinerama
            libXrandr

            gcc
            cmake
            gnumake
            ninja
            # ccache

            # For raygui package
            pkg-config
            pkgconf

            wayland-scanner
            gtk3
            libxkbcommon

            # clang-tools
            gdb-dashboard
          ];

          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath (with pkgs; [
            libpulseaudio
            wayland
            libxkbcommon
          ]);
        };
      }
    );
}

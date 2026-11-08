{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    libGL
    raylib
    # raygui

    # X11 dependencies
    libX11
    libX11.dev
    libXcursor
    libXi
    libXinerama
    libXrandr

    # Web support (uncomment to enable)
    emscripten

    gcc
    cmake
    gnumake
    ninja
    # ccache
    # pkg-config
    # pkgconf

    # clang-tools
  ];

  # Audio dependencies
  LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [pkgs.alsa-lib];
}

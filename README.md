[![Build Project - Linux](https://github.com/infgotoinf/hello-world/actions/workflows/build_linux.yml/badge.svg)](https://github.com/infgotoinf/hello-world/actions/workflows/build_linux.yml)
[![Build Project - Windows](https://github.com/infgotoinf/hello-world/actions/workflows/build_windows.yml/badge.svg)](https://github.com/infgotoinf/hello-world/actions/workflows/build_windows.yml)
[![Build Project - WebAssembly](https://github.com/infgotoinf/hello-world/actions/workflows/build_webassembly.yml/badge.svg)](https://github.com/infgotoinf/hello-world/actions/workflows/build_webassembly.yml)

![Hello, Wolrd!](./assets/art/interface/logo.png)

DOOM II clone written in C and Raylib

$(Game screenshots)

_[itch.io Release](https://link)_

## Build dependencies

> [!NOTE]
> You can use Raylib package unless you are on Windows. You need emscripten only if you build for the Web.

### Windows

You must install [MSYS2](https://www.msys2.org/) and launch **UCRT64** console in order to install dependencies and build the project:
```
pacman -S mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja make \
    mingw-w64-ucrt-x86_64-emscripten
```

### Generic GNU/Linux

Follow [official raylib guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)

### NixOS

Install [direnv](https://github.com/nix-community/nix-direnv) and run inside project folder:

```
direnv allow
```

## Build instructions

`make build` - build the game
`make run` - run the game
`make help` - view help options

## Made with

### Libraries and other assets

- [Raylib](https://www.raylib.com/index.html): Zlib
- [Raygui](https://github.com/raysan5/raygui): Zlib
- [GNU FreeFont](https://www.gnu.org/software/freefont/): GPL-3.0
- [GNU Unifont](https://www.unifoundry.com/unifont/index.html): OFL-1.1

### Programs

- [Krita](https://krita.org/)
- [Blender](https://www.blender.org/)
- [Renoise](https://www.renoise.com/)

## Controls

| Key | Action |
|-----|--------|
| AWSD   | Movement |
| Space  | Jump |
| Shift  | Run |
| Ctrl   | Crouch |
| LKM    | Attack/Shoot |
| R      | Reload |
| Esc    | Pause |
| `      | Show debug info |

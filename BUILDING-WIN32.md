Install [MSYS2](http://www.msys2.org/).

Open MSYS2 MSYS prompt (*not* MinGW 32-bit or 64-bit prompt!).

Install build tools from repo:
```sh
pacman -S base-devel make cmake mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain mingw-w64-i686-cmake mingw-w64-x86_64-cmake 
```

Install library dependencies from repo:
```sh
pacman -S mingw-w64-x64_64-libzip mingw-w64-x86_64-boost
```

Grab [MinHook PKGBUILD](https://github.com/int19h/MINGW-packages/tree/master/mingw-w64-MinHook),
and place it in a new empty directory somewhere. From that directory, do:
```sh
makepkg-mingw -sLf
pacman -U mingw-w64-x86_64-MinHook-1.3.3-1-any.pkg.tar.xz
```

Close MSYS prompt, and open MinGW 64-bit prompt.

Go to root directory of the repo, and run:
```sh
./build.sh
```

The resulting binary will be in `bin\Release`. To produce a debug build, do `./build.sh -d`, and
output will be in `bin\Debug`.

The following DLLs are runtime dependencies (can be copied from `C:\msys64\mingw64\bin`):
- libboost_filesystem-mt.dll
- libboost_program_options-mt.dll
- libboost_system-mt.dll
- libgcc_s_seh-1.dll
- libstdc++-6.dll
- libwinpthread-1.dll
- libzip-5.dll
- MinHook.dll

Install [MSYS2](http://www.msys2.org/).

Open MSYS2 MSYS prompt (*not* MinGW 32-bit or 64-bit prompt!).

Refresh package databse, and update packages (might need to close and restart shell after this)
```sh
pacman -Syu
```

Install build tools from repo:
```sh
pacman -S base-devel make cmake mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain mingw-w64-i686-cmake mingw-w64-x86_64-cmake 
```

Install library dependencies from repo:
```sh
pacman -S mingw-w64-x86_64-libzip mingw-w64-x86_64-boost mingw-w64-x86_64-MinHook
```

Fix missing libzip `zipconf.h` header:
```sh
cp /mingw64/lib/libzip/include/zipconf.h /mingw64/include/
```

Close MSYS prompt, and open regular Windows command prompt. Go to root directory of the repo, and run `build.cmd`.

The resulting binary will be in `bin\Release`, along with its runtime dependencies.
To produce a debug build, do `build.cmd -t Debug`, and output will be in `bin\Debug`.

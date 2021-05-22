- Install R 4.0+
- Mac: install XCode
- Ubuntu: install CLANG and CMake via apt.

Recommended:

- [VS Code](https://code.visualstudio.com/)
- [Microsoft C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools&ssr=false#overview). It helps you locate and configure kits. You can also build directly from VS Code.

### WSL

### Mac

R Host is Intel x64 only so you have to install and use x64 components. Consider this
[Installing Intel-based packages using Homebrew on the M1 Mac](https://www.wisdomgeek.com/development/installing-intel-based-packages-using-homebrew-on-the-m1-mac/)

- Install XCode
- Install XCode command line tools `xcode-select --install`

- Use Rosetta Terminal!
- Install Hmebrew from Rosetta terminal
- Install `cmake` via Homebrew: `brew install cmake`
- `brew install pkg-config`
- `brew install libzip`
- `brew install icu4c`
- `export CMAKE_PREFIX_PATH=/usr/local/opt/icu4c`

Related:

- [Missing libcudata issue with boost 1.75](https://github.com/Homebrew/homebrew-core/issues/67427)
- [Configuring CMake with boost 1.75](https://github.com/carlocab/macos-boost1.75-regex-bug)
- [Missing licudata](https://github.com/OpenRCT2/OpenRCT2/issues/8000)

### Install R

- `sudo apt-get install r-base`

### Install CMake

- `sudo apt remove --purge cmake`
- `sudo apt install cmake`
- `cmake --version`

### Install CLang

- `sudo apt install build-essential xz-utils curl`
- `curl -SL http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz | tar -xJC .`
- `clang --version`

### Install Boost from sources

- `cd ~`
- `wget -O boost_1_76_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.76.0/boost_1_76_0.tar.gz/download`
- `tar xzvf boost_1_76_0.tar.gz`
- `cd boost_1_76_0`
- `./bootstrap.sh --prefix=/usr/`
- `./b2`
- `sudo ./b2 install`

### Connect subtrees

- `git remote add picojson https://github.com/kazuho/picojson`
- `git subtree add --prefix lib/picojson picojson master`

### Configure CMake in VS Code

- `Ctrl+Shift+P`
- `CMake: Configure`

```
[main] Configuring folder: rhost
[proc] Executing command: /usr/bin/cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_C_COMPILER:FILEPATH=/bin/clang-10 -DCMAKE_CXX_COMPILER:FILEPATH=/bin/clang++-10 -H/home/mikhaila/rhost -B/home/mikhaila/rhost/build -G "Unix Makefiles"
[cmake] Not searching for unused variables given on the command line.
[cmake] -- The C compiler identification is Clang 10.0.0
[cmake] -- The CXX compiler identification is Clang 10.0.0
[cmake] -- Check for working C compiler: /bin/clang-10
[cmake] -- Check for working C compiler: /bin/clang-10 -- works
[cmake] -- Detecting C compiler ABI info
[cmake] -- Detecting C compiler ABI info - done
[cmake] -- Detecting C compile features
[cmake] -- Detecting C compile features - done
[cmake] -- Check for working CXX compiler: /bin/clang++-10
[cmake] -- Check for working CXX compiler: /bin/clang++-10 -- works
[cmake] -- Detecting CXX compiler ABI info
[cmake] -- Detecting CXX compiler ABI info - done
[cmake] -- Detecting CXX compile features
[cmake] -- Detecting CXX compile features - done
```

- `./build.sh`
- For packaging place output into the respective folder under `src/Host[Windows|Mac|Linux]`

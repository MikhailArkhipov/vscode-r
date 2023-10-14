- Install R 4.1+
- Mac: install XCode
- Ubuntu: install CLANG and CMake via apt.

Recommended:

- [VS Code](https://code.visualstudio.com/)
- [Microsoft C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools&ssr=false#overview). It helps you locate and configure kits. You can also build directly from VS Code.

### WSL (x86_64 only)

- `sudo apt-get update`
- `sudo apt install cmake`
- `sudo apt remove r-base* --purge`
- `sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9`
- `sudo add-apt-repository 'deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran40/'`
- `sudo apt install r-base`
- `sudo apt install -y libzip-dev`

## Ubuntu 18 (stick with boost 1.65)

- `wget -O boost_1_65_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.gz/download`
- `tar xzvf boost_1_65_0.tar.gz`
- `cd boost_1_65_0`
- `./bootstrap.sh --prefix=/usr/`
- `./b2`
- `sudo ./b2 install`

## Ubuntu 20:

- `wget -O boost_1_76_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.76.0/boost_1_76_0.tar.gz/download`
- `tar xzvf boost_1_76_0.tar.gz`
- `cd boost_1_76_0`
- `./bootstrap.sh --prefix=/usr/`
- `./b2`
- `sudo ./b2 install`

### Mac

### Starting with R 4.2 only ARM builds are supported 

- Install XCode 14.1+ (or update to newest)
- Install XCode command line tools `xcode-select --install` or remove and reinstall via `sudo rm -rf /Library/Developer/CommandLineTools`
- Install R 4.3+
- Install HomeBrew

- `brew install boost`
- `brew install libzip`
- `brew install icu4c`
- `brew install pkg-config`
- `brew install cmake`

Related:

- [Missing libcudata issue with boost 1.75](https://github.com/Homebrew/homebrew-core/issues/67427)
- [Configuring CMake with boost 1.75](https://github.com/carlocab/macos-boost1.75-regex-bug)
- [Missing licudata](https://github.com/OpenRCT2/OpenRCT2/issues/8000)

### Connect subtrees

- `git remote add picojson https://github.com/kazuho/picojson`
- `git subtree add --prefix lib/picojson picojson master`

### Configure CMake in VS Code

- `Ctrl+Shift+P`
- `CMake: Configure`

- `./build.sh 
- For packaging place output into the respective folder under `src/Host[Windows|Mac/x64|Mac/Arm64|Linux]`

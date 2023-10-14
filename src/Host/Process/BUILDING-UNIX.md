- Install R 4.1+
- Mac: install XCode
- Ubuntu: install CLANG and CMake via apt.

Recommended:

- [VS Code](https://code.visualstudio.com/)
- [Microsoft C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools&ssr=false#overview). It helps you locate and configure kits. You can also build directly from VS Code.

### WSL (x86_64 only)
Installing CMake, R and libzip:

- `sudo apt-get update`
- `sudo apt install cmake`
- `sudo apt remove r-base* --purge`
- `sudo apt install --no-install-recommends software-properties-common dirmngr`
- `wget -qO- https://cloud.r-project.org/bin/linux/ubuntu/marutter_pubkey.asc | sudo tee -a /etc/apt/trusted.gpg.d/cran_ubuntu_key.asc`
- `sudo add-apt-repository "deb https://cloud.r-project.org/bin/linux/ubuntu $(lsb_release -cs)-cran40/"`
- `sudo apt update`
- `sudo apt install r-base`
- `sudo apt install -y libzip-dev`

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

### Connect subtrees

- `git remote add picojson https://github.com/kazuho/picojson`
- `git subtree add --prefix lib/picojson picojson master`

### Configure CMake in VS Code

- `Ctrl+Shift+P`
- `CMake: Configure`

- `./build.sh 
- For packaging place output into the respective folder under `src/Host[Windows|Mac/x64|Mac/Arm64|Linux]`

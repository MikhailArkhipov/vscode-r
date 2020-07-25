Extension consists of three parts:
1. R Host process (C++, [R-Host repo](https://github.com/MikhailArkhipov/R-Host)) that loads `R.dll` and communicates over JSON RPC.
2. Language Server (most of the project, C#, .NET Core)
3. VS Code extension (TypeScript)

### Setup

1. Install Visual Studio 2019 Community with C#, C++ and .NET Core cross-platform development
2. Install VS Code
3. Clone the repo
4. Add R-Host as subtree
    - `git remote add RHost https://github.com/MikhailArkhipov/R-Host`
    - `git subtree add --prefix=src/Host/Process/ RHost master`
5. Add R-Host dependencies
    - `git remote add picojson https://github.com/kazuho/picojson`
    - `git subtree add --prefix src/Host/Process/lib/picojson picojson master`
5. R Host is binaries are checked in under `Host` subfolder

### Building R-Host
1. R Host is binaries are checked in under `Host` subfolder
2. If you want to change and build R-Host, you will have to do it for Windows, Linux and Mac separately on respective platforms. 
    - You can use WSL for Linux development on Windows. See [Building on Unix](https://github.com/MikhailArkhipov/R-Host/blob/master/BUILDING-UNIX.md) for details.
    - On Windows see [Building on Win32](https://github.com/MikhailArkhipov/R-Host/blob/master/BUILDING-WIN32.md).

### Building extension
1. cd `src/VSCode`
2. cd `npm install`
3. Open `src/VSCode` in VS Code
4. You should be able to run the extension and debug it

### Packaging
1. Install [vsce](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)
2. Run `package.cmd`
3. Test the resulting VSIX on Windows, Mac and Linux.


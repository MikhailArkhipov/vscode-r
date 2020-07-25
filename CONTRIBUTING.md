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
   - `git subtree pull --prefix src/Host/Process RHost master`
5. Add R-Host dependencies
   - `git remote add picojson https://github.com/kazuho/picojson`
   - `git subtree add --prefix src/Host/Process/lib/picojson picojson master`
   - `git subtree pull --prefix src/Host/Process/lib/picojson picojson master`
6. R Host is binaries are checked in under `Host/[Mac | Windows | Linux]` subfolder per platform.

### Building R-Host

This is usually unnecessary unless you are changing R Host. R Host binaries is binaries are checked in under `Host` subfolder per platform. Full instructions are at [R Host repo](https://github.com/MikhailArkhipov/R-Host).

- Windows
  [Building on Win32](https://github.com/MikhailArkhipov/R-Host/blob/master/BUILDING-WIN32.md).

- Mac/Linux
  [Building on Unix](https://github.com/MikhailArkhipov/R-Host/blob/master/BUILDING-UNIX.md). You can use WSL for Linux development on Windows.

### Building extension

1. cd `src/VSCode`
2. cd `npm install`
3. Open `src/VSCode` in VS Code
4. Copy language server bits from `bin/Debug/netcoreapp3.1` to `src/VSCode/ls` folder.
5. Copy `Host` folder to `ls` and replace R Host bits if you rebuilt it.
6. You should be able to run the extension.

### Debugging

1. Debugging TypeScript code can be done by launching extension in VS Code.
2. Debugging C# code:
3. Debugging R Host:

### Packaging

1. Install [vsce](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)
2. Run `package.cmd`
3. Test the resulting VSIX on Windows, Mac and Linux.

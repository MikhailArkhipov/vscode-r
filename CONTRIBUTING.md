Extension consists of three parts:

1. R Host process (C++, [R-Host repo](https://github.com/MikhailArkhipov/R-Host)) that loads `R.dll` and communicates over JSON RPC.
2. Language Server (most of the project, C#, .NET Core)
3. VS Code extension (TypeScript)

### Setup

1. Install Visual Studio 2022 Community with C#, C++ and .NET Core cross-platform development
2. Install VS Code
3. Clone the repo
4. Add R-Host dependencies (from `src/Host/Process`)
   - `git remote add picojson https://github.com/kazuho/picojson`
   - `git subtree add --prefix src/Host/Process/lib/picojson picojson master`
   - `git subtree pull --prefix src/Host/Process/lib/picojson picojson master`
4. R Host is binaries are checked in under `Host/[Mac | Windows | Linux]` subfolder per platform.

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
4. If you are planning to debug, change `Release` to `Debug` in `package.cmd`.
5. Run `debug.cmd`. This will populate `ls` folder with language server debug bits.
6. You should be able to run the extension.

### Debugging

1. Debugging TypeScript code can be done by launching extension in VS Code.
2. Debugging C# code:

   - Use Visual Studion `Debug | Attach To Process`. Attach to `dotnet.exe` running `Microsoft.R.LanguageServer.dll` (debug build).
   - If you need to debug startup sequence, open `Program.cs` in the `Microsoft.R.LanguageServer` and uncomment `#define WAIT_FOR_DEBUGGER`. Language server with then stop at startup and wait for the debugger to attach.
   - Same applies for `Microsoft.R.Broker` if you need to debug it.

3. Debugging R Host:

- Attach to `Microsoft.R.Host` process using either C++ tools in Visual Studio or VS Code.
- If you need to debug startup sequence, add some wait to the `main.cpp`.

C# broker process and R Host have tracing facilities that can be enabled. See the source code.

### Packaging

1. Install [vsce](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)
2. Run `package.cmd`
3. Test the resulting VSIX on Windows, Mac and Linux.

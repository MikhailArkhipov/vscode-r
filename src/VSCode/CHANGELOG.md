# Changelog

## 0.0.11 (28 July 2020)

### Updates

1. Implement `r.terminalPath` setting which allows user to provide separate path to R binaries and to executable for the R Terminal. For example, one may choose to use [Radian](https://github.com/randy3k/radian) in the terminal. The extension completions engine still needs to know where to find R binaries so it is recommended to set `r.interpreterPath` to whatever version of R used in the terminal. Addresses [#34](https://github.com/MikhailArkhipov/vscode-r/issues/34).

## 0.0.10 (26 July 2020)

### Updates

1. Limited support for plots in internal window. See the next item.
2. Provide `r.execute` command (`Ctrl+Shift+Enter`) to evaluate code in internal R session. This may be useful when working in remote container, such as WSL, since external windows do not work in remote session as it is headless. Executing command over a line that yields a plot will output plot in the new internal `Plot` window. **Remember though that terminal window and internal R session are not connected, so executing code in Terminal and then attempting plot the result via internal session won't work.** With remote ressions, you can execute code in Terminal, save results into a file, then execute plotting in internal session providing results from the file.
3. Remove dependency on `libzip` on Mac and Linux.
4. Fix issue when R didn't start with `Permission denied` on Mac and Linux.
5. Fixed issue with TLS key expiration ["R session process did not start on the machine 'VSCR'. Exception: Key has expired"](https://github.com/MikhailArkhipov/vscode-r/issues/30). HTTPS requirement was removed since all sessions are local to the machine.

## 0.0.9 (28 June 2020)

### Updates

1. Provide `r.interpreterPath` manual setting for cases when R does not get discovered automatically.

## 0.0.8 (01 May 2020)

### Updates

1. Add support for R 4.0.

## 0.0.7 (22 Mar 2020)

### Updates

1. Upgrade .NET Core to 3.1.

## 0.0.6 (10 Feb 2019)

### Fixes

1. Fixes [#10](https://github.com/MikhailArkhipov/vscode-r/issues/10) - excessive CPU consumption by Microsoft.R.Host process on Mac and Linux.
2. Fixes [#15](https://github.com/MikhailArkhipov/vscode-r/issues/15) - paths with whitespace are quoted incorrectly.
3. Adds troubleshooting section to README.

## 0.0.5 (7 Dec 2018)

### Fixes

1. Fixes [#6](https://github.com/MikhailArkhipov/vscode-r/issues/6) - ':' and '\$' are not completion triggers.

2. Fixes [#11](https://github.com/MikhailArkhipov/vscode-r/issues/11) - 'source' file on Windows should be replacing backslashes with forward slashes.

3. Fixes [#12](https://github.com/MikhailArkhipov/vscode-r/issues/12) - duplicate 'source' command was removed.

## 0.0.3 (26 Nov 2018)

### Fixes

1. Fixes [#2](https://github.com/MikhailArkhipov/vscode-r/issues/2) - `libzip` library may be missing on MacOS preventing R Host from starting. Extension now automates installation of libzip on MacOS via [Homebrew](https://brew.sh/). If /usr/local/opt/libzip/lib/libzip.5.dylib is missing, extension tries to locate Homebrew and install the library by running `brew install libzip`.

2. Fixes [#5](https://github.com/MikhailArkhipov/vscode-r/issues/5) - occasional null reference exception during language server startup.

### Improvements

1. Add support for R Markdown files. R Tools extension is activated when \*.rmd file is opened. It automatically associates RMD files with the VS Code markdown editor. Since R Tools extension is active, Ctrl+Enter allows executing R code fragments in the R Markdown files in R Terminal. See also [#3](https://github.com/MikhailArkhipov/vscode-r/issues/3)

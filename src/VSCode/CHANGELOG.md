# Changelog

## 0.0.28 (10 May 2022)

1. Downgrade to .NET 5.0 for Mac comatibility. Mac M1 still has to use .NET 5.0 x64 b/c of breaking changes in .NET 6.0 ARM64.

## 0.0.27 (9 May 2022)

1. Update to .NET 6.0
2. Support native pipe operator per R 4.1. Closes [#82](https://github.com/MikhailArkhipov/vscode-r/issues/82). Note that pipe operator will not be flagged as error in lower R versions. Per version support may be added in the future.
3. Security updates in Node and Nuget modules.

## 0.0.26 (17 Aug 2021)

1. Limit keybindings to R editor so they do not activate in other languages. Closes [#75](https://github.com/MikhailArkhipov/vscode-r/issues/75)
2. Security updates in Node modules.

## 0.0.25 (16 June 2021)

1. Support ARM R 4.1 on Apple M1 silicon. **VS Code for ARM is required**. Closes [#69](https://github.com/MikhailArkhipov/vscode-r/issues/69)

**Make sure you install VS Code for OSX for ARM**. You may have to download it separately since if you have had VS Code installed on M1 before ARM build became available and kept upgrading it, it is probably still x64. The extension by default uses ARM version with R 4.1 or higher. However, if you need to use Intel build of R such as when you may be using native packages not available for ARM, you can select x64 R from the list of interpreters - such as pick '4.1' and not '4.1-arm64'. Use `r.interpreter` setting to specify the desired interpreter index.

2. Build Linux host for Ubuntu 18 so both 18 and 20 are supported. Fixes [#72](https://github.com/MikhailArkhipov/vscode-r/issues/72)
3. Security updates in npm modules.

## 0.0.24 (22 May 2021)

1. Add support for R 4.1: update native R Host to reflect changes in R API. The host is still Intel x64. M1 ARM 64 support is yet to be done.

## 0.0.23 (21 Apr 2021)

1. Update references per github security bot recommendation. No logic changes.

## 0.0.22 (13 Apr 2021)

1. Suppress cancellation exception when looking up document symbols. Fixes [#54](https://github.com/MikhailArkhipov/vscode-r/issues/54).

## 0.0.21 (12 Apr 2021)

1. Adds support for R 4.x raw strings.

## 0.0.20 (24 Feb 2021)

1. Fixes occasional exception when editing files.
2. Adds diagnostic output for internal exceptions, if any.

## 0.0.19 (19 Feb 2021)

1. Implement hierarchical display of the document symbols in the Outline window.

## 0.0.18 (15 Feb 2021)

1. Add support for multiple terminals (fixes [#47](https://github.com/MikhailArkhipov/vscode-r/issues/47)). 'Open R Terminal' command opens or creates R terminal. New 'Create R Terminal' command always creates another copy. If active terminal is closed, last used instance, if any, will be activated on sending code to the terminal.

## 0.0.17 (13 Feb 2021)

1. Fixes [#53](https://github.com/MikhailArkhipov/vscode-r/issues/53). Extension now activates when R terminal opens.
2. Fixes issues running on MacOS 11.2 with Rosetta

## 0.0.16 (05 Feb 2021)

1. Move to .NET 5.0.

## 0.0.15 (13 Sep 2020)

1. Fixes [#37](https://github.com/MikhailArkhipov/vscode-r/issues/37). `Ctrl+Enter` and `source` now send code to internal R session as well, effectively duplicating the evaluation there. This allows tracking variable assignment in the editor. Code needs to be sourced for variables to become available to the completion engine.

<table>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<img src="https://user-images.githubusercontent.com/12820357/93031083-02b1b680-f5dd-11ea-8391-7bb0a6ff3547.png" width=500 />
</td></tr>
</table>

2. Fixed issue preventing sourcing to terminal in some cases.
3. Removed unnecessary package dependencies.
4. Requre VS Code 1.49+ for security reasons.

## 0.0.14 (6 Aug 2020)

Simplified output of process exit code when broker or R host doesn't start for diagnostics purposes.

## 0.0.13 (1 Aug 2020)

Fixed calculation of active parameter and its range in the signature string [#9](https://github.com/MikhailArkhipov/vscode-r/issues/9).

## 0.0.12 (29 July 2020)

Added shortcuts for `<-` and `%>%` as suggested in [#22](https://github.com/MikhailArkhipov/vscode-r/issues/22). Also, added completion triggers on `%` and `<` which allows using completion list for snippets, as in [RTVS](https://github.com/microsoft/rtvs).

You can either type `<` and then `TAB` to get `<-` via completion list or use keyboard shortcut `alt+-`. For pipe `%>%` either type `%` and then `TAB` or use `ctrl+shift+m` shortcut. Snippets file can be found in `snippets/r.json` in the extension folder.

Thanks to [@jackbrookes](https://github.com/jackbrookes) for suggestions.

## 0.0.11 (28 July 2020)

1. Implemented `r.terminalPath` setting which allows user to provide separate path to R binaries and to executable for the R Terminal.

You may choose to use [Radian](https://github.com/randy3k/radian) in the terminal. The extension completions engine still needs to know where to find R binaries so it is recommended to set `r.interpreterPath` to whatever version of R used in the terminal. Addresses [#34](https://github.com/MikhailArkhipov/vscode-r/issues/34).

## 0.0.10 (26 July 2020)

1. Added limited support for plots in internal window. See the next item.

2. Provided `r.execute` command (`Ctrl+Shift+Enter`) to evaluate code in internal R session. This may be useful when working in remote container, such as WSL, since external windows do not work in remote session as it is headless. Executing command over a line that yields a plot will output plot in the new internal `Plot` window. **Remember though that terminal window and internal R session are not connected, so executing code in Terminal and then attempting plot the result via internal session won't work.** With remote ressions, you can execute code in Terminal, save results into a file, then execute plotting in internal session providing results from the file.

3. Removed dependency on `libzip` on Mac and Linux.

4. Fixed issue when R didn't start with `Permission denied` on Mac and Linux.

5. Fixed issue with TLS key expiration ["R session process did not start on the machine 'VSCR'. Exception: Key has expired"](https://github.com/MikhailArkhipov/vscode-r/issues/30). HTTPS requirement was removed since all sessions are local to the machine.

## 0.0.9 (28 June 2020)

Provided `r.interpreterPath` manual setting for cases when R does not get discovered automatically.

## 0.0.8 (01 May 2020)

Added support for R 4.0.

## 0.0.7 (22 Mar 2020)

Upgraded .NET Core to 3.1.

## 0.0.6 (10 Feb 2019)

1. Fixed [#10](https://github.com/MikhailArkhipov/vscode-r/issues/10) - excessive CPU consumption by Microsoft.R.Host process on Mac and Linux.
2. Fixed [#15](https://github.com/MikhailArkhipov/vscode-r/issues/15) - paths with whitespace are quoted incorrectly.
3. Added troubleshooting section to README.

## 0.0.5 (7 Dec 2018)

1. Fixed [#6](https://github.com/MikhailArkhipov/vscode-r/issues/6) - ':' and '\$' are not completion triggers.

2. Fixed [#11](https://github.com/MikhailArkhipov/vscode-r/issues/11) - 'source' file on Windows should be replacing backslashes with forward slashes.

3. Fixed [#12](https://github.com/MikhailArkhipov/vscode-r/issues/12) - duplicate 'source' command was removed.

## 0.0.3 (26 Nov 2018)

1. Fixed [#2](https://github.com/MikhailArkhipov/vscode-r/issues/2) - `libzip` library may be missing on MacOS preventing R Host from starting. Extension now automates installation of libzip on MacOS via [Homebrew](https://brew.sh/). If /usr/local/opt/libzip/lib/libzip.5.dylib is missing, extension tries to locate Homebrew and install the library by running `brew install libzip`.

2. Fixed [#5](https://github.com/MikhailArkhipov/vscode-r/issues/5) - occasional null reference exception during language server startup.

3. Added support for R Markdown files. R Tools extension is activated when \*.rmd file is opened. It automatically associates RMD files with the VS Code markdown editor. Since R Tools extension is active, Ctrl+Enter allows executing R code fragments in the R Markdown files in R Terminal. See also [#3](https://github.com/MikhailArkhipov/vscode-r/issues/3)

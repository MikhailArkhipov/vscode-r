# R extension for Visual Studio Code

A [Visual Studio Code](https://code.visualstudio.com/) [extension](https://marketplace.visualstudio.com/VSCode) provides support for the [R language](https://www.r-project.org/) for R 3.2+. Features include syntax checking, completions, code formatting, formatting as you type, tooltips, linting.

---

### Supported platforms

-   Windows x64
-   Mac OS 10.12+
-   Linux distros supported by [.NET 5.0 Runtime](https://www.microsoft.com/net/download).

---

![Completions](https://raw.githubusercontent.com/MikhailArkhipov/vscode-r/master/src/VSCode/images/ggplot.png)

---

![Completions](https://raw.githubusercontent.com/MikhailArkhipov/vscode-r/master/src/VSCode/images/completions.png)

---

## Prerequsites

-   [.NET 5.0 Runtime](https://www.microsoft.com/net/download)
-   [R distribution (64-bit only)](https://cloud.r-project.org/)

## Quick start

Open the Command Palette (Command+Shift+P on macOS and Ctrl+Shift+P on Windows/Linux) and type 'R:' to see list of available commands and shortcuts.

### _Syntax check_

Syntax check is performed as you type or when opening a file. Look for red squiggles. Problems are also reported in the `Problems` tab.

### _Formatting_

Extension provides ability to format document or selection. Formatting options are available in the `r.editor` settings section.

### _Automatic formatting (as you type)_

The editor can format code after you type Enter, ; or }. You can control the feature via `editor.formatOnType` and `r.editor.fornatOnType` settings. Formatting settings are the same as in the document formatting.

### _Linting_

Functionality is close to [lintr](https://github.com/jimhester/lintr). However, you do not have to run linting explicitly, it happens as you type. By default it is disabled, you can enable it by setting `r.linting.enable` to `true`. Linter has various options available in the `r.linting` section.

### _Execute in terminal/source file_

Use `R: Execute line or selection` commands to execute line or selection in the terminal. Similarly, use `R: source file` to source file from the editor into the terminal section.

### Shortcuts and snippets

Extension provides shortcuts for `<-` and `%>%` as suggested in [#22](https://github.com/MikhailArkhipov/vscode-r/issues/22). There are completion triggers on `%` and `<` which allows using completion list for snippets, as in [RTVS](https://github.com/microsoft/rtvs).

You can either type `<` and then `TAB` to get `<-` via completion list or use keyboard shortcut `alt+-`. For pipe `%>%` either type `%` and then `TAB` or use `ctrl+shift+m` shortcut.

Snippets file can be found in `snippets/r.json` in the extension folder.

Thanks to [@jackbrookes](https://github.com/jackbrookes) for suggestions and snippets.

### Remoting and WSL

The extension does work in WSL. However, output windows cannot be displayed since UI does not translate over remote connection. There is limited support for plotting though. Remoting like `R Tools in Visual Studio` into containers with preinstalled RTVS broker is not supported. Please use VS Code remoting instead.

### Plotting

Plots typically appear in external R windows. However, there is support for internal `Plot` window that may come handy when working with remote sessions, such as WSL. Try `r.execute` command (`Ctrl+Shift+Enter`) to evaluate code in internal R session. Executing command over a line that yields a plot will output plot in the internal `Plot` window.

Remember though that **Terminal window and internal R session are not connected**. Executing code in Terminal and then attempting plot the result via internal session won't work. With remote ressions, you can execute code in Terminal, save results into a file, then execute plotting in internal session providing results from the file.

![Internal Plot Window](https://user-images.githubusercontent.com/12820357/88484757-cd1a1680-cf25-11ea-93a6-3af4d697f6d1.png))

### Using custom R executable in terminal

You can set `r.terminalPath` to an executable for the R Terminal. For example, [Radian](https://github.com/randy3k/radian). The extension completions engine still needs to know where to find R binaries so it is recommended to set `r.interpreterPath` to whatever version of R used in the terminal.

## Known issues

R session in the editor does not automatically pick up new packages installed in the terminal. You may have to reload the window for the session to pick up newly installed modules.

## Troubleshooting

**.NET Core not found**

You can bypass .NET Core check by setting

```json
"r.dependencyChecks": false
```

**R Interpreter not found**

Try using `r.intepreterPath` setting and specify path to R installation. The path should be root folder, like `C:\Program Files\R\R-3.4.3`, without `bin/x64`.

## Not currently supported

-   Debugging
-   Completions in console
-   32-bit R
-   Dynamic update of the session when new packages are installed.

## Bug reports

Please file issues at the project [GitHub](https://github.com/MikhailArkhipov/vscode-r)

## Disclaimer

-   Extension is based on Microsoft R Tools for Visual Studio aka RTVS.
    Please visit https://github.com/microsoft/rtvs for more information.

-   This project is a personal effort and although it builds upon Microsoft code,
    it is not supported by Microsoft. I work on the code occasionally in my spare time.

# R extension for Visual Studio Code

A [Visual Studio Code](https://code.visualstudio.com/) [extension](https://marketplace.visualstudio.com/VSCode) provides support for the [R language](https://www.r-project.org/) for R 3.2+. Features include syntax checking, completions, code formatting, formatting as you type, tooltips, linting.

## Prerequsites

- [.NET Core 2.1 Runtime](https://www.microsoft.com/net/download)
- [R distribution (64-bit only)](https://cloud.r-project.org/)

## Quick start

Open the Command Palette (Command+Shift+P on macOS and Ctrl+Shift+P on Windows/Linux) and type 'R:' to see list of available commands and shortcuts.

## Not currently supported
- Debugging
- Remote or Docker connectivity
- Completions in console
- In-app Plot or other graphical output
- 32-bit R

## Bug reports
Please file issues at the project [GitHub](https://github.com/MikhailArkhipov/vscode-r)

## Disclaimer
- Extension is based on Microsoft R Tools for Visual Studio aka RTVS.
Please visit https://github.com/microsoft/rtvs for more information.
Prototype extension to VS Code can be found in the RTVS repo. 

- This project is a personal effort and although it builds upon RTVS,
it is not supported by Microsoft. I work on the code occasionally 
in my spare time.

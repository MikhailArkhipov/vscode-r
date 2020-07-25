rd /s /q bin\Debug
cd src\Host\Broker\Impl
dotnet publish -c Debug
cd ..\..\..\LanguageServer\Impl
dotnet publish -c Debug
cd ..\..\VsCode
rd /s /q ls
md ls
md ls\Host
cd ..\..
xcopy /S bin\Debug\netcoreapp3.1\publish\*.* src\VsCode\ls
xcopy /S Host src\VsCode\ls\Host
rem cd src\VsCode
rem vsce package
rem cd ..\..

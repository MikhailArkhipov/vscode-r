rd /s /q bin\Release
cd src\Host\Broker\Impl
dotnet publish -c Release
cd ..\..\..\LanguageServer\Impl
dotnet publish -c Release
cd ..\..\VsCode
rd /s /q ls
md ls
md ls\Host
cd ..\..
xcopy /S bin\Release\netcoreapp3.1\publish\*.* src\VsCode\ls
del src\VsCode\ls\*.pdb
xcopy /S Host src\VsCode\ls\Host
cd src\VsCode
vsce package
cd ..\..

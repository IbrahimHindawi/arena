@echo off
mkdir bin
pushd bin
cl /nologo ..\src\main.c /Zi
popd
bin\main.exe

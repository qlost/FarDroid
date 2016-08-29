@echo off
setlocal enabledelayedexpansion 

perl syncVersion.pl

set zip="%programfiles%\7-Zip\7z.exe"
set msbuild="%programfiles(x86)%\MSBuild\14.0\Bin\MSBuild.exe"
set project=..\fardroid.sln

for /F "tokens=3*" %%i  in (..\version.info) do set version=!version!.%%i
set version=!version:~1!

rd /q /s ..\Release
del /q *.7z

for %%p in (x64 x86) do (
	set arch=fardroid_%version%_%%p.7z
	rd /q /s %%p\

	%msbuild% %project% /p:Platform="%%p" /p:Configuration=Release /nologo /v:m
	IF ERRORLEVEL 1 EXIT /B 1

	xcopy /s /y /Exclude:exclude.txt ..\Release\%%p\* %%p\FarDroid\ | findstr /e /c:"copied"
	IF ERRORLEVEL 1 EXIT /B 1

	cd %%p
	%zip% a ..\!arch! FarDroid | findstr /b /c:"Everything is Ok" /c:"Creating archive"
	IF ERRORLEVEL 1 EXIT /B 1
	cd ..
)

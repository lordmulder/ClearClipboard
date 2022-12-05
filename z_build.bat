@echo off
setlocal enabledelayedexpansion

REM ///////////////////////////////////////////////////////////////////////////
REM // Set Paths
REM ///////////////////////////////////////////////////////////////////////////

set "MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC"

REM ###############################################
REM # DO NOT MODIFY ANY LINES BELOW THIS LINE !!! #
REM ###############################################

REM ///////////////////////////////////////////////////////////////////////////
REM // Setup environment
REM ///////////////////////////////////////////////////////////////////////////

cd /d "%~dp0"

if not exist "%JAVA_HOME%\bin\java.exe" (
	echo Java could not be found, please make sure JAVA_HOME is set correctly!
	goto BuildError
)

call "%MSVC_PATH%\vcvarsall.bat" x86

if "%VCINSTALLDIR%"=="" (
	echo %%VCINSTALLDIR%% not specified. Please check your MSVC_PATH var!
	goto BuildError
)
if not exist "%VCINSTALLDIR%\bin\cl.exe" (
	echo C++ compiler not found. Please check your MSVC_PATH var!
	goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Get current date and time (in ISO format)
REM ///////////////////////////////////////////////////////////////////////////

set "ISO_DATE="
if not exist "%~dp0\..\Prerequisites\MSYS\1.0\bin\date.exe" goto BuildError

for /F "tokens=1,2 delims=:" %%a in ('"%~dp0\..\Prerequisites\MSYS\1.0\bin\date.exe" +ISODATE:%%Y-%%m-%%d') do (
	if "%%a"=="ISODATE" set "ISO_DATE=%%b"
)

if "%ISO_DATE%"=="" goto BuildError

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the binaries
REM ///////////////////////////////////////////////////////////////////////////

echo ---------------------------------------------------------------------
echo BEGIN BUILD
echo ---------------------------------------------------------------------

for %%p in (Win32, x64) do (
	MSBuild.exe /property:Platform=%%p /property:Configuration=Release /target:clean   "%~dp0\ClearClipboard.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	MSBuild.exe /property:Platform=%%p /property:Configuration=Release /target:rebuild "%~dp0\ClearClipboard.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	MSBuild.exe /property:Platform=%%p /property:Configuration=Release /target:build   "%~dp0\ClearClipboard.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Create documents
REM ///////////////////////////////////////////////////////////////////////////

for %%i in ("%~dp0\*.md") do (
	echo PANDOC: %%~nxi
	"%~dp0\..\Prerequisites\Pandoc\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes+implicit_figures --to html5 --toc --toc-depth=2 -N --standalone -H "%~dp0\..\Prerequisites\Pandoc\css\github-pandoc.inc" "%%~i" | "%JAVA_HOME%\bin\java.exe" -jar "%~dp0\..\Prerequisites\HTMLCompressor\bin\htmlcompressor-1.5.3.jar" --compress-css -o "%%~dpni.html"
	if not "!ERRORLEVEL!"=="0" (
		goto BuildError
	)
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Copy base files
REM ///////////////////////////////////////////////////////////////////////////

echo ---------------------------------------------------------------------
echo BEGIN PACKAGING
echo ---------------------------------------------------------------------

set "PACK_PATH=%TMP%\~%RANDOM%%RANDOM%.tmp"

mkdir "%PACK_PATH%"
mkdir "%PACK_PATH%\x64"

copy "%~dp0\bin\Win32\Release\*.exe" "%PACK_PATH%"
copy "%~dp0\bin\.\x64\Release\*.exe" "%PACK_PATH%\x64"
copy "%~dp0\*.txt" "%PACK_PATH%"
copy "%~dp0\*.html" "%PACK_PATH%"

REM ///////////////////////////////////////////////////////////////////////////
REM // Attributes
REM ///////////////////////////////////////////////////////////////////////////

attrib +R "%PACK_PATH%\*.exe"
attrib +R "%PACK_PATH%\*.html"
attrib +R "%PACK_PATH%\*.txt"

REM ///////////////////////////////////////////////////////////////////////////
REM // Generate outfile name
REM ///////////////////////////////////////////////////////////////////////////

mkdir "%~dp0\out"
set "OUT_NAME=ClearClipboard.%ISO_DATE%"

:CheckOutName
if exist "%~dp0\out\%OUT_NAME%.zip" (
	set "OUT_NAME=%OUT_NAME%.new"
	goto CheckOutName
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the package
REM ///////////////////////////////////////////////////////////////////////////

pushd "%PACK_PATH%
"%~dp0\..\Prerequisites\InfoZip\zip.exe" -9 -r -z "%~dp0\out\%OUT_NAME%.zip" "*.*" < "%~dp0\Copying.txt"
popd

attrib +R "%~dp0\out\%OUT_NAME%.zip"
rmdir /Q /S "%PACK_PATH%"

REM ///////////////////////////////////////////////////////////////////////////
REM // COMPLETE
REM ///////////////////////////////////////////////////////////////////////////

echo.
echo Build completed.
echo.

pause
goto:eof

REM ///////////////////////////////////////////////////////////////////////////
REM // FAILED
REM ///////////////////////////////////////////////////////////////////////////

:BuildError

echo.
echo Build has failed !!!
echo.
pause

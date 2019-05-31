@echo off
setlocal enabledelayedexpansion

if not exist "%JAVA_HOME%\bin\java.exe" (
	echo Java could not be found, please make sure JAVA_HOME is set correctly!
	pause && goto:eof
)

for %%i in ("%~dp0\*.md") do (
	echo PANDOC: %%~nxi
	"%~dp0\..\Prerequisites\Pandoc\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes+implicit_figures --to html5 --toc -N --standalone -H "%~dp0\..\Prerequisites\Pandoc\css\github-pandoc.inc" "%%~i" | "%JAVA_HOME%\bin\java.exe" -jar "%~dp0\..\Prerequisites\HTMLCompressor\bin\htmlcompressor-1.5.3.jar" --compress-css -o "%%~dpni.html"
	if not "!ERRORLEVEL!"=="0" (
		"%~dp0\..\Prerequisites\CEcho\cecho.exe" red "\nSomething went wrong^^!\n"
		pause && exit
	)
)

echo.
pause

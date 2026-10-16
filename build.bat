@echo off
setlocal
REM Builds x64\Release\LLMAssistant.dll with the newest Visual Studio (2022 or
REM later, any edition or Build Tools) that has the C++ build tools.
REM   build.bat            build, then wait for a key press
REM   build.bat /nopause   build without waiting (used by deploy.bat)

set "ROOT=%~dp0"
set "NO_PAUSE="
if /i "%~1"=="/nopause" set "NO_PAUSE=1"

REM Work from the repo folder, also when started elevated (cwd is System32 then)
cd /d "%ROOT%"

echo Building Notepad++ LLM Assistant Plugin...

REM Find the newest Visual Studio with the x64 C++ compiler
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio not found: "%VSWHERE%" does not exist.
    echo Install Visual Studio 2022 or later with the "Desktop development with C++" workload.
    goto :fail
)
set "VS_PATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH (
    echo ERROR: No Visual Studio installation with the C++ build tools was found.
    echo Add the "Desktop development with C++" workload in the Visual Studio Installer.
    goto :fail
)
if not exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
    echo ERROR: "%VS_PATH%\Common7\Tools\VsDevCmd.bat" not found.
    goto :fail
)

echo Using Visual Studio at %VS_PATH%
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -no_logo -startdir=none

REM Check if MSBuild is available after setting up environment
where msbuild >nul 2>nul
if errorlevel 1 (
    echo ERROR: MSBuild not found after setting up "%VS_PATH%".
    goto :fail
)

REM Keep the project's platform toolset when this Visual Studio has it;
REM otherwise build with the newest toolset it does have, e.g. v143 on
REM VS 2022 while the project asks for v145.
set "PROJECT_TOOLSET="
for /f "tokens=2,3 delims=<>" %%a in ('findstr /c:"<PlatformToolset>" LLMAssistant.vcxproj') do (
    if "%%a"=="PlatformToolset" if not defined PROJECT_TOOLSET set "PROJECT_TOOLSET=%%b"
)
set "TOOLSET_FOUND="
set "NEWEST_TOOLSET="
for /d %%v in ("%VS_PATH%\MSBuild\Microsoft\VC\v*") do (
    for /d %%t in ("%%v\Platforms\x64\PlatformToolsets\v*") do call :note_toolset %%~nxt
)
set "TOOLSET_ARG="
if defined TOOLSET_FOUND (
    echo Platform toolset: %PROJECT_TOOLSET%
) else if defined NEWEST_TOOLSET (
    echo Toolset "%PROJECT_TOOLSET%" from LLMAssistant.vcxproj is not installed, so %NEWEST_TOOLSET% is used instead.
    set "TOOLSET_ARG=/p:PlatformToolset=%NEWEST_TOOLSET%"
) else (
    echo ERROR: No C++ platform toolset found under "%VS_PATH%\MSBuild\Microsoft\VC".
    goto :fail
)

echo.
msbuild LLMAssistant.vcxproj /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0 %TOOLSET_ARG%
if errorlevel 1 goto :fail
if not exist "x64\Release\LLMAssistant.dll" (
    echo ERROR: MSBuild finished but x64\Release\LLMAssistant.dll was not produced.
    goto :fail
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo The plugin DLL is located at: x64\Release\LLMAssistant.dll
if defined NO_PAUSE exit /b 0
echo.
echo Installation steps:
echo 1. Create folder: C:\Program Files\Notepad++\plugins\LLMAssistant\
echo 2. Copy LLMAssistant.dll to that folder
echo 3. Restart Notepad++
echo 4. Configure your OpenAI API key in Plugins ^> LLM Assistant ^> Settings
echo ========================================
pause
exit /b 0

:fail
echo.
echo ========================================
echo BUILD FAILED!
echo ========================================
echo Please check the error messages above.
echo Make sure you have:
echo - Visual Studio 2022 or later with the "Desktop development with C++" workload
echo - A Windows SDK, which that workload installs
echo ========================================
if not defined NO_PAUSE pause
exit /b 1

REM Records one installed toolset folder name, such as v145
:note_toolset
if /i "%~1"=="%PROJECT_TOOLSET%" set "TOOLSET_FOUND=1"
if "%~1" GTR "%NEWEST_TOOLSET%" set "NEWEST_TOOLSET=%~1"
exit /b 0

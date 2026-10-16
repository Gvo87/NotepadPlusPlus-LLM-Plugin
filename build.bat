@echo off
setlocal
REM Builds <platform>\Release\LLMAssistant.dll with the newest Visual Studio
REM (2022 or later, any edition or Build Tools) that has the C++ build tools for
REM that platform.
REM   build.bat                build for 64-bit Notepad++ (x64\Release), then wait for a key press
REM   build.bat x86            build for 32-bit Notepad++ (Win32\Release); Win32 works too
REM   build.bat arm64          build for ARM64 Notepad++ (ARM64\Release)
REM   build.bat x64 /nopause   build without waiting (used by deploy.bat)

set "ROOT=%~dp0"
set "PLATFORM=x64"
set "NO_PAUSE="
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="/nopause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="x64" (
    set "PLATFORM=x64"
) else if /i "%~1"=="x86" (
    set "PLATFORM=Win32"
) else if /i "%~1"=="Win32" (
    set "PLATFORM=Win32"
) else if /i "%~1"=="arm64" (
    set "PLATFORM=ARM64"
) else (
    echo ERROR: Unknown argument "%~1".
    echo Usage: build.bat [x64^|x86^|arm64] [/nopause]
    goto :fail
)
shift
goto :parse_args
:args_done

REM Work from the repo folder, also when started elevated (cwd is System32 then)
cd /d "%ROOT%"

echo Building Notepad++ LLM Assistant Plugin for %PLATFORM%...

REM x64 and Win32 come from the same Visual Studio component; ARM64 needs its own.
set "VC_COMPONENT=Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
if "%PLATFORM%"=="ARM64" set "VC_COMPONENT=Microsoft.VisualStudio.Component.VC.Tools.ARM64"

REM Find the newest Visual Studio with the C++ compiler for this platform
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio not found: "%VSWHERE%" does not exist.
    echo Install Visual Studio 2022 or later with the "Desktop development with C++" workload.
    goto :fail
)
set "VS_PATH="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires %VC_COMPONENT% -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH if "%PLATFORM%"=="ARM64" (
    echo ERROR: No Visual Studio installation with the ARM64 C++ build tools was found.
    echo In the Visual Studio Installer, choose Modify, open Individual components and add
    echo "MSVC Build Tools for ARM64/ARM64EC (Latest)". On Visual Studio 2022 it is called
    echo "MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (Latest)".
    goto :fail
)
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
    for /d %%t in ("%%v\Platforms\%PLATFORM%\PlatformToolsets\v*") do call :note_toolset %%~nxt
)
set "TOOLSET_ARG="
if defined TOOLSET_FOUND (
    echo Platform toolset: %PROJECT_TOOLSET%
) else if defined NEWEST_TOOLSET (
    echo Toolset "%PROJECT_TOOLSET%" from LLMAssistant.vcxproj is not installed, so %NEWEST_TOOLSET% is used instead.
    set "TOOLSET_ARG=/p:PlatformToolset=%NEWEST_TOOLSET%"
) else (
    echo ERROR: No %PLATFORM% C++ platform toolset found under "%VS_PATH%\MSBuild\Microsoft\VC".
    goto :fail
)

set "DLL=%PLATFORM%\Release\LLMAssistant.dll"
echo.
msbuild LLMAssistant.vcxproj /p:Configuration=Release /p:Platform=%PLATFORM% /p:WindowsTargetPlatformVersion=10.0 %TOOLSET_ARG%
if errorlevel 1 goto :fail
if not exist "%DLL%" (
    echo ERROR: MSBuild finished but %DLL% was not produced.
    goto :fail
)

REM 32-bit Notepad++ installs to Program Files (x86) on 64-bit Windows
set "NPP_DIR=C:\Program Files\Notepad++"
if "%PLATFORM%"=="Win32" set "NPP_DIR=C:\Program Files (x86)\Notepad++"

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo The plugin DLL is located at: %DLL%
if defined NO_PAUSE exit /b 0
echo.
echo Installation steps:
echo 1. Create folder: %NPP_DIR%\plugins\LLMAssistant\
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
echo - For arm64 builds, the ARM64/ARM64EC build tools component
echo ========================================
if not defined NO_PAUSE pause
exit /b 1

REM Records one installed toolset folder name, such as v145
:note_toolset
if /i "%~1"=="%PROJECT_TOOLSET%" set "TOOLSET_FOUND=1"
if "%~1" GTR "%NEWEST_TOOLSET%" set "NEWEST_TOOLSET=%~1"
exit /b 0

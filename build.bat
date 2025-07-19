@echo off
echo Building Notepad++ LLM Assistant Plugin with Visual Studio 2022...

REM Set Visual Studio 2022 environment
set VS2022_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
set VS2022_PRO_PATH="C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
set VS2022_ENT_PATH="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"

REM Try to find and setup VS2022 environment
if exist %VS2022_PATH% (
    echo Found Visual Studio 2022 Community
    call %VS2022_PATH%
) else if exist %VS2022_PRO_PATH% (
    echo Found Visual Studio 2022 Professional
    call %VS2022_PRO_PATH%
) else if exist %VS2022_ENT_PATH% (
    echo Found Visual Studio 2022 Enterprise
    call %VS2022_ENT_PATH%
) else (
    echo Visual Studio 2022 not found in standard locations.
    echo Please run this from a Visual Studio 2022 Developer Command Prompt.
    pause
    exit /b 1
)

REM Check if MSBuild is available after setting up environment
where msbuild >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo MSBuild not found. Please ensure Visual Studio 2022 is properly installed.
    pause
    exit /b 1
)

echo Using MSBuild from Visual Studio 2022...

REM Build the plugin with VS2022 toolset
msbuild LLMAssistant.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    echo The plugin DLL is located at: x64\Release\LLMAssistant.dll
    echo.
    echo Installation steps:
    echo 1. Create folder: C:\Program Files\Notepad++\plugins\LLMAssistant\
    echo 2. Copy LLMAssistant.dll to that folder
    echo 3. Restart Notepad++
    echo 4. Configure your OpenAI API key in Plugins ^> LLM Assistant ^> Settings
    echo ========================================
) else (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    echo Please check the error messages above.
    echo Make sure you have:
    echo - Visual Studio 2022 with C++ development tools
    echo - Windows 10 SDK
    echo ========================================
)

pause

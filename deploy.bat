@echo off
echo ========================================
echo LLM Assistant Plugin - Auto Deploy
echo ========================================

echo.
echo [1/4] Building plugin...
echo Setting up Visual Studio 2022 environment...

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
    echo ERROR: Visual Studio 2022 not found in standard locations.
    echo Please run this from a Visual Studio 2022 Developer Command Prompt.
    pause
    exit /b 1
)

REM Check if MSBuild is available after setting up environment
where msbuild >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MSBuild not found. Please ensure Visual Studio 2022 is properly installed.
    pause
    exit /b 1
)

echo Building with MSBuild...
msbuild LLMAssistant.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)
echo Build completed successfully!

echo.
echo [2/4] Closing Notepad++...
taskkill /f /im notepad++.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Notepad++ closed successfully
    timeout /t 2 /nobreak >nul
) else (
    echo Notepad++ was not running
)

echo.
echo [3/4] Creating plugin directory...
if not exist "C:\Program Files\Notepad++\plugins\LLMAssistant\" (
    mkdir "C:\Program Files\Notepad++\plugins\LLMAssistant\"
    echo Plugin directory created
) else (
    echo Plugin directory already exists
)

echo.
echo [4/4] Deploying plugin DLL...
copy /Y "x64\Release\LLMAssistant.dll" "C:\Program Files\Notepad++\plugins\LLMAssistant\"
if %ERRORLEVEL% equ 0 (
    echo Plugin deployed successfully!
) else (
    echo ERROR: Failed to copy plugin DLL
    echo Make sure you run this as Administrator
    pause
    exit /b 1
)

echo.
echo ========================================
echo DEPLOYMENT COMPLETE!
echo ========================================
echo.
echo The plugin has been successfully deployed to:
echo C:\Program Files\Notepad++\plugins\LLMAssistant\LLMAssistant.dll
echo.
echo You can now start Notepad++ to use the updated plugin.
echo.
echo Would you like to start Notepad++ now? (Y/N)
set /p choice=
if /i "%choice%"=="Y" (
    echo Starting Notepad++...
    start "" "C:\Program Files\Notepad++\notepad++.exe"
)

echo.
echo Press any key to exit...
pause >nul

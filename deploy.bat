@echo off
setlocal
REM Builds the plugin and copies it into C:\Program Files\Notepad++\plugins.
REM Run it as Administrator, since it writes to Program Files.

REM Work from the repo folder, also when started elevated (cwd is System32 then)
cd /d "%~dp0"

echo ========================================
echo LLM Assistant Plugin - Auto Deploy
echo ========================================

REM Check for admin rights first, so a run without them stops before it
REM builds anything or closes Notepad++.
fltmc >nul 2>&1
if errorlevel 1 (
    echo ERROR: Run this script as Administrator, because it copies the plugin
    echo into C:\Program Files\Notepad++. Right-click deploy.bat and choose
    echo "Run as administrator".
    pause
    exit /b 1
)

echo.
echo [1/4] Building plugin...
call "%~dp0build.bat" /nopause
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)
echo Build completed successfully!

echo.
echo [2/4] Closing Notepad++...
call :close_notepadpp
if errorlevel 1 (
    echo Deployment cancelled. The plugin was not copied.
    pause
    exit /b 1
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
exit /b 0

REM Closes Notepad++ the way its own close button does (taskkill without /f
REM sends WM_CLOSE), so it can ask to save unsaved files. Returns 1 if
REM Notepad++ is still running and the user cancels.
:close_notepadpp
call :notepadpp_running
if errorlevel 1 (
    echo Notepad++ was not running
    exit /b 0
)
echo Asking Notepad++ to close. If it asks to save changes, answer it to continue.
taskkill /im notepad++.exe >nul 2>&1
for /l %%i in (1,1,20) do (
    timeout /t 1 /nobreak >nul
    call :notepadpp_running
    if errorlevel 1 (
        echo Notepad++ closed successfully
        exit /b 0
    )
)
:close_notepadpp_wait
echo.
echo Notepad++ is still running. It may be showing a save prompt, or a save
echo was cancelled. Close Notepad++ yourself, saving your work as needed.
choice /c RC /n /m "Press R once Notepad++ is closed, or C to cancel: "
if errorlevel 2 exit /b 1
call :notepadpp_running
if errorlevel 1 (
    echo Notepad++ closed successfully
    exit /b 0
)
goto :close_notepadpp_wait

REM Sets errorlevel 0 if notepad++.exe is running, 1 if not
:notepadpp_running
tasklist /fi "imagename eq notepad++.exe" /nh 2>nul | findstr /i /l /c:"notepad++.exe" >nul
if errorlevel 1 exit /b 1
exit /b 0

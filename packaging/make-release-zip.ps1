<#
.SYNOPSIS
    Builds a Notepad++ Plugins-Admin-ready zip for LLM Assistant and prints the
    SHA-256 + a ready-to-paste nppPluginList entry for one Notepad++ architecture.

.DESCRIPTION
    Notepad++ Plugins Admin requires:
      * a downloadable .zip,
      * with the plugin DLL at the ROOT of the zip,
      * named exactly LLMAssistant.dll,
      * and the SHA-256 of that zip recorded as "id" in nppPluginList.

    Each Notepad++ architecture has its own plugin list and needs its own DLL:

      -Platform   DLL built from     zip                           nppPluginList
      x64         x64\Release        LLMAssistant_v<ver>_x64.zip   src/pl.x64.json
      x86         Win32\Release      LLMAssistant_v<ver>_x86.zip   src/pl.x86.json
      arm64       ARM64\Release      LLMAssistant_v<ver>_arm64.zip src/pl.arm64.json

    Before packaging, the script checks that the DLL is built for that
    architecture, that its version resource matches -Version, and that it exports
    the functions Notepad++ looks up when it loads a plugin. It then packages the
    DLL into that exact layout and emits the metadata you need for the PR.

.PARAMETER Platform
    Notepad++ architecture: x64 (default), x86 or arm64. Win32 also means x86.

.PARAMETER Version
    Plugin version, e.g. "1.0". Must match the DLL's binary version resource and
    the git tag (vVersion). Defaults to reading src\Version.h. A version that is
    not a number, such as "dev" for a CI test build, only names the zip and skips
    the version check.

.PARAMETER DllPath
    Path to the built DLL. Defaults to <MSBuild platform>\Release\LLMAssistant.dll,
    e.g. Win32\Release\LLMAssistant.dll for -Platform x86.

.PARAMETER OutDir
    Where to write the zip. Defaults to .\dist.

.EXAMPLE
    pwsh packaging\make-release-zip.ps1 -Version 1.0

.EXAMPLE
    pwsh packaging\make-release-zip.ps1 -Platform x86
#>
[CmdletBinding()]
param(
    [ValidateSet("x64", "x86", "arm64", "Win32")]
    [string]$Platform = "x64",
    [string]$Version,
    [string]$DllPath,
    [string]$OutDir  = "dist"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

# Notepad++ name -> MSBuild platform (output folder) and PE machine type.
$platforms = @{
    "x64"   = @{ MSBuild = "x64";   Machine = 0x8664 }
    "x86"   = @{ MSBuild = "Win32"; Machine = 0x014C }
    "arm64" = @{ MSBuild = "ARM64"; Machine = 0xAA64 }
}
$Platform = $Platform.ToLowerInvariant()
if ($Platform -eq "win32") { $Platform = "x86" }
$target = $platforms[$Platform]

# The functions Notepad++ looks up with GetProcAddress when it loads a plugin.
$requiredExports = @("beNotified", "getFuncsArray", "getName", "isUnicode", "messageProc", "setInfo")

function Read-VersionFromHeader {
    $header = Join-Path $repoRoot "src\Version.h"
    if (-not (Test-Path $header)) { return $null }
    $text = Get-Content $header -Raw
    $maj = [regex]::Match($text, 'LLM_VERSION_MAJOR\s+(\d+)').Groups[1].Value
    $min = [regex]::Match($text, 'LLM_VERSION_MINOR\s+(\d+)').Groups[1].Value
    if ($maj -and $min) { return "$maj.$min" }
    return $null
}

# Reads the machine type and the exported function names from a PE file.
function Read-PeInfo([string]$Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    if ([BitConverter]::ToUInt32($bytes, $pe) -ne 0x00004550) { throw "'$Path' is not a PE file." }
    $coff = $pe + 4
    $machine = [BitConverter]::ToUInt16($bytes, $coff)
    $sectionCount = [BitConverter]::ToUInt16($bytes, $coff + 2)
    $optional = $coff + 20
    $sections = $optional + [BitConverter]::ToUInt16($bytes, $coff + 16)
    # The data directories start 96 bytes into a PE32 optional header and 112
    # bytes into a PE32+ (64-bit) one. The first directory is the export table.
    $is64 = [BitConverter]::ToUInt16($bytes, $optional) -eq 0x20B
    $exportRva = [BitConverter]::ToUInt32($bytes, $optional + $(if ($is64) { 112 } else { 96 }))

    function ConvertTo-FileOffset([long]$Rva) {
        for ($i = 0; $i -lt $sectionCount; $i++) {
            $section = $sections + 40 * $i
            $start = [BitConverter]::ToUInt32($bytes, $section + 12)
            $size = [Math]::Max([BitConverter]::ToUInt32($bytes, $section + 8), [BitConverter]::ToUInt32($bytes, $section + 16))
            if ($Rva -ge $start -and $Rva -lt $start + $size) {
                return $Rva - $start + [BitConverter]::ToUInt32($bytes, $section + 20)
            }
        }
        throw "RVA $Rva is outside every section of '$Path'."
    }

    $exports = @()
    if ($exportRva -ne 0) {
        $directory = ConvertTo-FileOffset $exportRva
        $nameCount = [BitConverter]::ToUInt32($bytes, $directory + 24)
        $nameTable = ConvertTo-FileOffset ([BitConverter]::ToUInt32($bytes, $directory + 32))
        for ($i = 0; $i -lt $nameCount; $i++) {
            $name = ConvertTo-FileOffset ([BitConverter]::ToUInt32($bytes, $nameTable + 4 * $i))
            $end = [Array]::IndexOf($bytes, [byte]0, [int]$name)
            $exports += [System.Text.Encoding]::ASCII.GetString($bytes, $name, $end - $name)
        }
    }
    [pscustomobject]@{ Machine = $machine; Exports = $exports }
}

if (-not $Version) {
    $Version = Read-VersionFromHeader
    if (-not $Version) { throw "Could not determine version. Pass -Version explicitly (e.g. -Version 1.0)." }
    Write-Host "Using version from src\Version.h: $Version"
}

if (-not $DllPath) { $DllPath = "$($target.MSBuild)\Release\LLMAssistant.dll" }
if (-not (Test-Path $DllPath)) {
    throw "DLL not found at '$DllPath'. Build the Release|$($target.MSBuild) configuration first (build.bat $Platform), or pass -DllPath."
}
$DllPath = (Resolve-Path $DllPath).Path

# Check the DLL before packaging it: Notepad++ can't load a DLL built for another
# architecture, and Plugins Admin compares the version resource to the entry.
$pe = Read-PeInfo $DllPath
if ($pe.Machine -ne $target.Machine) {
    $actual = ($platforms.GetEnumerator() | Where-Object { $_.Value.Machine -eq $pe.Machine } | Select-Object -ExpandProperty Key)
    if (-not $actual) { $actual = "machine type 0x{0:X4}" -f $pe.Machine }
    throw "'$DllPath' is built for $actual, not $Platform."
}
$missing = $requiredExports | Where-Object { $_ -cnotin $pe.Exports }
if ($missing) {
    throw "'$DllPath' does not export $($missing -join ', '). Exports found: $($pe.Exports -join ', ')"
}
$fileInfo = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($DllPath)
$dllVersion = [version]::new($fileInfo.FileMajorPart, $fileInfo.FileMinorPart, $fileInfo.FileBuildPart, $fileInfo.FilePrivatePart)
if ($Version -match '^\d+(\.\d+){0,3}$') {
    $expected = [version](($Version.Split('.') + @('0', '0', '0') | Select-Object -First 4) -join '.')
    if ($dllVersion -ne $expected) {
        throw "'$DllPath' has version $dllVersion, but -Version is $Version. Bump src\Version.h and rebuild, or pass the matching -Version."
    }
} else {
    Write-Host "Version '$Version' is not a number, so the DLL version ($dllVersion) is not checked against it."
}
Write-Host ("Checked {0}: {1} (0x{2:X4}), version {3}, exports {4}" -f $DllPath, $Platform, $pe.Machine, $dllVersion, ($requiredExports -join ', '))

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Stage the DLL at the zip root under the exact required name.
$staging = Join-Path $OutDir "_staging"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force -Path $staging | Out-Null
Copy-Item $DllPath (Join-Path $staging "LLMAssistant.dll") -Force

$zipName = "LLMAssistant_v${Version}_${Platform}.zip"
$zipPath = Join-Path $OutDir $zipName
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $staging "*") -DestinationPath $zipPath -Force
Remove-Item $staging -Recurse -Force

$sha = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToUpper()

# Guard the git lookup: on machines without Git in PATH this must not abort the
# script (ErrorActionPreference is "Stop"); fall back to the known repo URL.
$repoUrl = $null
if (Get-Command git -ErrorAction SilentlyContinue) {
    try { $repoUrl = (& git config --get remote.origin.url) 2>$null } catch { $repoUrl = $null }
}
if ($repoUrl) { $repoUrl = $repoUrl -replace '\.git$', '' } else { $repoUrl = "https://github.com/Gvo87/NotepadPlusPlus-LLM-Plugin" }
$downloadUrl = "$repoUrl/releases/download/v$Version/$zipName"

Write-Host ""
Write-Host "===================================================================="
Write-Host " Package : $zipPath"
Write-Host " SHA-256 : $sha"
Write-Host "===================================================================="
Write-Host ""
Write-Host "Add this entry to nppPluginList  ->  src/pl.$Platform.json  (npp-plugins array):"
Write-Host ""

$entry = [ordered]@{
    "folder-name"  = "LLMAssistant"
    "display-name" = "LLM Assistant"
    "version"      = $Version
    "id"           = $sha
    "repository"   = $downloadUrl
    "description"  = "AI-powered text processing (enhance, translate, summarize, grammar, code) via the OpenAI API, directly inside Notepad++."
    "author"       = "Gvo87"
    "homepage"     = $repoUrl
}

$entry | ConvertTo-Json | Write-Output

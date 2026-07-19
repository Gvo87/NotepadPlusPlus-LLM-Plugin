<#
.SYNOPSIS
    Builds a Notepad++ Plugins-Admin-ready zip for LLM Assistant and prints the
    SHA-256 + a ready-to-paste nppPluginList (pl.x64.json) entry.

.DESCRIPTION
    Notepad++ Plugins Admin requires:
      * a downloadable .zip,
      * with the plugin DLL at the ROOT of the zip,
      * named exactly LLMAssistant.dll,
      * and the SHA-256 of that zip recorded as "id" in nppPluginList.

    This script packages an already-built DLL (x64\Release\LLMAssistant.dll by
    default) into that exact layout, then emits the metadata you need for the PR.

.PARAMETER Version
    Plugin version, e.g. "1.0". Must match the DLL's binary version resource and
    the git tag (vVersion). Defaults to reading src\Version.h.

.PARAMETER DllPath
    Path to the built DLL. Defaults to x64\Release\LLMAssistant.dll.

.PARAMETER OutDir
    Where to write the zip. Defaults to .\dist.

.EXAMPLE
    pwsh packaging\make-release-zip.ps1 -Version 1.0
#>
[CmdletBinding()]
param(
    [string]$Version,
    [string]$DllPath = "x64\Release\LLMAssistant.dll",
    [string]$OutDir  = "dist"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Read-VersionFromHeader {
    $header = Join-Path $repoRoot "src\Version.h"
    if (-not (Test-Path $header)) { return $null }
    $text = Get-Content $header -Raw
    $maj = [regex]::Match($text, 'LLM_VERSION_MAJOR\s+(\d+)').Groups[1].Value
    $min = [regex]::Match($text, 'LLM_VERSION_MINOR\s+(\d+)').Groups[1].Value
    if ($maj -and $min) { return "$maj.$min" }
    return $null
}

if (-not $Version) {
    $Version = Read-VersionFromHeader
    if (-not $Version) { throw "Could not determine version. Pass -Version explicitly (e.g. -Version 1.0)." }
    Write-Host "Using version from src\Version.h: $Version"
}

if (-not (Test-Path $DllPath)) {
    throw "DLL not found at '$DllPath'. Build the Release|x64 configuration first (build.bat), or pass -DllPath."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Stage the DLL at the zip root under the exact required name.
$staging = Join-Path $OutDir "_staging"
if (Test-Path $staging) { Remove-Item $staging -Recurse -Force }
New-Item -ItemType Directory -Force -Path $staging | Out-Null
Copy-Item $DllPath (Join-Path $staging "LLMAssistant.dll") -Force

$zipName = "LLMAssistant_v${Version}_x64.zip"
$zipPath = Join-Path $OutDir $zipName
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $staging "*") -DestinationPath $zipPath -Force
Remove-Item $staging -Recurse -Force

$sha = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToUpper()

$repoUrl = (git config --get remote.origin.url) 2>$null
if ($repoUrl) { $repoUrl = $repoUrl -replace '\.git$', '' } else { $repoUrl = "https://github.com/Gvo87/NotepadPlusPlus-LLM-Plugin" }
$downloadUrl = "$repoUrl/releases/download/v$Version/$zipName"

Write-Host ""
Write-Host "===================================================================="
Write-Host " Package : $zipPath"
Write-Host " SHA-256 : $sha"
Write-Host "===================================================================="
Write-Host ""
Write-Host "Add this entry to nppPluginList  ->  src/pl.x64.json  (npp-plugins array):"
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

# Project Wire — One-Line Installer for Windows
# Usage (PowerShell): irm https://raw.githubusercontent.com/Suprath/Wire/main/install.ps1 | iex

$ErrorActionPreference = "Stop"

$Repo    = "Suprath/Wire"
$Asset   = "wire-windows-x64.zip"
$InstDir = "$env:LOCALAPPDATA\Wire"

Write-Host ""
Write-Host "  ╔══════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "  ║     Project Wire — Windows Installer     ║" -ForegroundColor Cyan
Write-Host "  ╚══════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# ── Get Latest Release ────────────────────────────────────────────────────────
Write-Host "  Fetching latest release from GitHub..." -ForegroundColor Yellow
$ApiUrl  = "https://api.github.com/repos/$Repo/releases/latest"
$Release = Invoke-RestMethod -Uri $ApiUrl -Headers @{ "User-Agent" = "WireInstaller" }
$Tag     = $Release.tag_name

if (-not $Tag) {
    Write-Host "  [!] Could not fetch release info. Check your internet." -ForegroundColor Red
    exit 1
}

Write-Host "  Latest version: $Tag" -ForegroundColor Green

# ── Download ──────────────────────────────────────────────────────────────────
$DownloadUrl = "https://github.com/$Repo/releases/download/$Tag/$Asset"
$TmpZip      = "$env:TEMP\wire-windows-x64.zip"
$TmpExtract  = "$env:TEMP\wire-extract"

Write-Host "  Downloading $Asset..." -ForegroundColor Yellow
Invoke-WebRequest -Uri $DownloadUrl -OutFile $TmpZip -UseBasicParsing

# ── Extract ───────────────────────────────────────────────────────────────────
Write-Host "  Extracting..." -ForegroundColor Yellow
if (Test-Path $TmpExtract) { Remove-Item $TmpExtract -Recurse -Force }
Expand-Archive -Path $TmpZip -DestinationPath $TmpExtract -Force

# ── Install ───────────────────────────────────────────────────────────────────
New-Item -ItemType Directory -Path $InstDir -Force | Out-Null
Copy-Item "$TmpExtract\wire_tui.exe" "$InstDir\wire_tui.exe" -Force
Copy-Item "$TmpExtract\wire_tui_interactive.exe" "$InstDir\wire_tui_interactive.exe" -Force

# ── Unblock executables (removes SmartScreen warning) ─────────────────────────
Unblock-File "$InstDir\wire_tui.exe"
Unblock-File "$InstDir\wire_tui_interactive.exe"

# ── Add to User PATH ──────────────────────────────────────────────────────────
$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($CurrentPath -notlike "*$InstDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$CurrentPath;$InstDir", "User")
    Write-Host "  Added Wire to PATH." -ForegroundColor Green
}

# ── Add Windows Firewall Rule (UDP 9001 & 9002) ───────────────────────────────
Write-Host "  Configuring firewall..." -ForegroundColor Yellow
$FwRule = Get-NetFirewallRule -DisplayName "Wire UDP" -ErrorAction SilentlyContinue
if (-not $FwRule) {
    try {
        New-NetFirewallRule -DisplayName "Wire UDP" -Direction Inbound `
            -Protocol UDP -LocalPort 9001,9002 -Action Allow | Out-Null
        Write-Host "  Firewall rule added (UDP 9001, 9002)." -ForegroundColor Green
    } catch {
        Write-Host "  [!] Could not add firewall rule automatically." -ForegroundColor Yellow
        Write-Host "      Re-run this installer as Administrator to fix it." -ForegroundColor Yellow
    }
}

# ── Cleanup ───────────────────────────────────────────────────────────────────
Remove-Item $TmpZip -Force -ErrorAction SilentlyContinue
Remove-Item $TmpExtract -Recurse -Force -ErrorAction SilentlyContinue

# ── Done ──────────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "  ✓ Wire $Tag installed successfully!" -ForegroundColor Green
Write-Host "  Location: $InstDir" -ForegroundColor White
Write-Host ""
Write-Host "  To start Wire:" -ForegroundColor Cyan
Write-Host "    Close this window and open a NEW PowerShell, then run:" -ForegroundColor White
Write-Host "      wire_tui_interactive" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Or run it right now:" -ForegroundColor Cyan
Write-Host "      `$env:PEER_NAME='Peer_A'; & '$InstDir\wire_tui_interactive.exe'" -ForegroundColor Cyan
Write-Host ""

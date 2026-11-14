# Adds the otv* commands (tools/otv.ps1) to your PowerShell profile and puts
# an "OTV" shortcut on the desktop that opens the otv menu.
# Run once:  powershell -ExecutionPolicy Bypass -File tools/install_otv_commands.ps1
# Then open a new terminal and type: otv   (or double-click OTV on the desktop)

$otv = Join-Path $PSScriptRoot 'otv.ps1'
$line = ". '$otv'"
$profilePath = $PROFILE.CurrentUserCurrentHost

if (-not (Test-Path $profilePath)) {
    New-Item -ItemType File -Path $profilePath -Force | Out-Null
}
$existing = Get-Content $profilePath -Raw
if ($existing -and $existing.Contains($otv)) {
    Write-Host "Profile: already installed in $profilePath" -ForegroundColor Green
} else {
    Add-Content -Path $profilePath -Value "`n# --- OTV commands (Team H2Woah!) ---`n$line" -Encoding utf8
    Write-Host "Profile: installed in $profilePath" -ForegroundColor Green
}

# Desktop shortcut. Loads otv.ps1 itself, so it works even if the profile doesn't.
$desktop = [Environment]::GetFolderPath('Desktop')
$lnkPath = Join-Path $desktop 'OTV.lnk'
$shell = New-Object -ComObject WScript.Shell
$lnk = $shell.CreateShortcut($lnkPath)
$lnk.TargetPath = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
$lnk.Arguments = "-NoExit -ExecutionPolicy Bypass -Command `". '$otv'; otv`""
$lnk.WorkingDirectory = Split-Path -Parent $PSScriptRoot
$lnk.Description = 'OTV menu (Team H2Woah!)'
$ide = Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\Arduino IDE.exe'
if (Test-Path $ide) { $lnk.IconLocation = "$ide,0" } else { $lnk.IconLocation = "$env:SystemRoot\System32\shell32.dll,12" }
$lnk.Save()
Write-Host "Shortcut: $lnkPath" -ForegroundColor Green
Write-Host 'Open a new terminal and type: otv   (or double-click OTV on the desktop)'

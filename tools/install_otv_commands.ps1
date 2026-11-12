# Adds the otv* commands (tools/otv.ps1) to your PowerShell profile.
# Run once:  powershell -ExecutionPolicy Bypass -File tools/install_otv_commands.ps1
# Then open a new terminal and type: otvhelp

$otv = Join-Path $PSScriptRoot 'otv.ps1'
$line = ". '$otv'"
$profilePath = $PROFILE.CurrentUserCurrentHost

if (-not (Test-Path $profilePath)) {
    New-Item -ItemType File -Path $profilePath -Force | Out-Null
}
$existing = Get-Content $profilePath -Raw
if ($existing -and $existing.Contains($otv)) {
    Write-Host "Already installed in $profilePath" -ForegroundColor Green
} else {
    Add-Content -Path $profilePath -Value "`n# --- OTV commands (Team H2Woah!) ---`n$line" -Encoding utf8
    Write-Host "Installed in $profilePath. Open a new terminal and type: otvhelp" -ForegroundColor Green
}

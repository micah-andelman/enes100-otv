# Copies teammate sketches from the team Google Drive folder into teammates/.
# Run from the repo root:
#   powershell -ExecutionPolicy Bypass -File tools/pull_teammates.ps1
#
# Needs Google Drive for desktop, with the shared "ENES 100-H2Woah" folder
# added to My Drive as a shortcut (Drive web: right-click the folder >
# Organize > Add shortcut > My Drive). Override the path with -DriveFolder.
#
# Only code files (.ino .h .cpp) are copied, from any subfolder, into
# teammates/ (flat). Nothing is deleted. Review with `git diff teammates/`
# before porting, and commit the originals.

param(
    [string]$DriveFolder = 'G:\My Drive\ENES 100-H2Woah'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root 'teammates'

if (-not (Test-Path -LiteralPath $DriveFolder)) {
    throw "Drive folder not found: $DriveFolder`nAdd the shared folder to My Drive as a shortcut, or pass -DriveFolder."
}

# (-Include is ignored with -LiteralPath in Windows PowerShell 5.1, so filter here.)
$files = Get-ChildItem -LiteralPath $DriveFolder -Recurse -File |
    Where-Object { @('.ino', '.h', '.cpp') -contains $_.Extension.ToLower() }

# teammates/ is flat, so two files with the same name in different
# subfolders would overwrite each other. Stop instead of guessing.
$dupes = $files | Group-Object Name | Where-Object { $_.Count -gt 1 }
if ($dupes) {
    foreach ($d in $dupes) { Write-Host "Same name in more than one place: $($d.Name)"; $d.Group | ForEach-Object { Write-Host "  $($_.FullName)" } }
    throw 'Rename or remove the duplicates in Drive, then run again.'
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null
$new = 0; $updated = 0; $same = 0
foreach ($f in $files) {
    $target = Join-Path $dest $f.Name
    if (-not (Test-Path -LiteralPath $target)) {
        Copy-Item -LiteralPath $f.FullName -Destination $target
        Write-Host "NEW      $($f.Name)"
        $new++
    } elseif ((Get-FileHash -LiteralPath $f.FullName).Hash -ne (Get-FileHash -LiteralPath $target).Hash) {
        Copy-Item -LiteralPath $f.FullName -Destination $target -Force
        Write-Host "UPDATED  $($f.Name)"
        $updated++
    } else {
        $same++
    }
}
Write-Host "Done: $new new, $updated updated, $same unchanged (from $DriveFolder)"

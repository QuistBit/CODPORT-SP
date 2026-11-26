param(
    [string]$GameDirectory,
    [ValidateSet('single', 'multi')][string]$ThreadMode = 'multi'
)
$ErrorActionPreference = 'Stop'

if (-not $GameDirectory) {
    Add-Type -AssemblyName System.Windows.Forms
    $picker = New-Object System.Windows.Forms.OpenFileDialog
    try {
        $picker.Title = 'Select CoDSP.exe in your original Call of Duty installation'
        $picker.Filter = 'Call of Duty single-player (CoDSP.exe)|CoDSP.exe'
        if ($picker.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { exit 0 }
        $GameDirectory = [IO.Path]::GetDirectoryName($picker.FileName)
    } finally {
        $picker.Dispose()
    }
}

$game = (Get-Item -LiteralPath $GameDirectory).FullName
foreach ($required in @('CoDSP.exe', 'main\pak0.pk3')) {
    if (-not (Test-Path -LiteralPath (Join-Path $game $required) -PathType Leaf)) {
        throw "Missing $required in the selected Call of Duty installation: $game"
    }
}
$runtime = Join-Path $PSScriptRoot 'bin'
$profile = Join-Path $PSScriptRoot 'profile'
$smp = if ($ThreadMode -eq 'single') { '0' } else { '1' }
$workers = if ($ThreadMode -eq 'single') { '0' } else { '-1' }
$compute = if ($ThreadMode -eq 'single') { '0' } else { '1' }
$engineArgs = @(
    '+set', 'fs_basepath', $profile, '+set', 'fs_homepath', $profile,
    '+set', 'fs_cdpath', $game, '+set', 'fs_game', 'main',
    '+set', 'r_fullscreen', '0', '+set', 'r_mode', '-1',
    '+set', 'r_customwidth', '800', '+set', 'r_customheight', '600',
    '+set', 'r_ignorehwgamma', '1', '+set', 'sys_cpuMHz', '3000',
    '+set', 'vm_game', '0', '+set', 'vm_cgame', '0', '+set', 'vm_ui', '0',
    '+set', 'com_introplayed', '1', '+set', 'com_maxfps', '60',
    '+set', 'r_smp', $smp, '+set', 'r_swapInterval', '0',
    '+set', 'com_multithreading', $compute, '+set', 'com_workerThreads', $workers,
    '+set', 'com_asyncLog', '1',
    '+set', 'developer', '0', '+set', 'logfile', '2', '+exec', 'training_player.cfg'
)
Write-Host "Threading mode: $ThreadMode"
Push-Location -LiteralPath $runtime
try {
    & (Join-Path $runtime 'WolfSP.exe') @engineArgs | Out-Null
    $gameExit = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $gameExit

param(
    [string]$GameDirectory,
    [ValidateSet('single', 'multi')][string]$ThreadMode = 'multi',
    [ValidateSet('legacy', 'native')][string]$CampaignMode = 'legacy',
    [switch]$PrintArguments
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
$profileName = if ($CampaignMode -eq 'native') { 'profile-native' } else { 'profile' }
$profile = Join-Path $PSScriptRoot $profileName
if ($CampaignMode -eq 'native' -and -not (Test-Path -LiteralPath (Join-Path $profile 'main\wolfconfig.cfg') -PathType Leaf)) {
    throw 'Native profile missing. Build R47F or later from source in a new output directory.'
}
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
    '+set', 'developer', '0', '+set', 'logfile', '2'
)
if ($CampaignMode -eq 'native') {
    # Preserve the non-GSC values from the accepted Training control.
    # No training_player.cfg execution or g_cod1Gsc feature assignments.
    $engineArgs += @(
        '+set', 'sv_framerate_smoothing', '1',
        '+set', 'r_cod1MaterialTrace', '0', '+set', 'r_cod1ImageTrace', '0',
        '+set', 'r_cod1StaticLighting', '1', '+set', 'r_cod1StaticLightDebug', '0',
        '+set', 'r_cod1PointLighting', '0', '+set', 'cg_debugEvents', '0',
        '+set', 'cg_footsteps', '1', '+set', 'r_cod1EfxShaders', '1',
        '+set', 'dmflags', '0', '+set', 'cg_r196FoleyScene', '0',
        '+set', 'cg_r197FoleyTransition', '0', '+set', 'logfile', '1',
        '+set', 'g_cod1CampaignProfile', '1'
    )
} else {
    $engineArgs += @('+exec', 'training_player.cfg')
}
if ($PrintArguments) {
    ConvertTo-Json -InputObject $engineArgs -Compress
    exit 0
}
Write-Host "Threading mode: $ThreadMode; campaign mode: $CampaignMode; profile: $profile"
Push-Location -LiteralPath $runtime
try {
    & (Join-Path $runtime 'WolfSP.exe') @engineArgs | Out-Null
    $gameExit = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $gameExit

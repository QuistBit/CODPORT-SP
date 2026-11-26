#requires -Version 5.1
<#
.SYNOPSIS
Build the Windows x86 Training profile without Python.
.DESCRIPTION
Finds the installed Visual Studio C++ tools, initializes their x86 environment,
and uses the same JSON recipe and historical headers as build_windows.py.
.EXAMPLE
.\Build-Windows.cmd -Output build_opt_r30
.EXAMPLE
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\build_windows.ps1 -Output build_opt_r30
#>
[CmdletBinding()]
param(
    [string]$Output,
    [string]$GamePgd,
    [string]$OpenAL,
    [string]$VisualStudio
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Resolve-BuildPath {
    param([Parameter(Mandatory = $true)][string]$Path)
    # Unlike Path.GetFullPath, this follows PowerShell's current location.
    return $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path)
}

function ConvertTo-WindowsArgument {
    param([Parameter(Mandatory = $true)][AllowEmptyString()][string]$Value)
    # Windows CRT quoting, also used by MSVC response files. Backslashes before
    # a quote or the closing quote need doubling; other backslashes stay literal.
    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    $quoted = New-Object System.Text.StringBuilder
    [void]$quoted.Append('"')
    $slashes = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -eq [char]92) {
            $slashes++
        } elseif ($character -eq [char]34) {
            [void]$quoted.Append([char]92, (2 * $slashes + 1))
            [void]$quoted.Append($character)
            $slashes = 0
        } else {
            [void]$quoted.Append([char]92, $slashes)
            [void]$quoted.Append($character)
            $slashes = 0
        }
    }
    [void]$quoted.Append([char]92, (2 * $slashes))
    [void]$quoted.Append('"')
    return $quoted.ToString()
}

function Invoke-CapturedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FileName,
        [string]$ArgumentLine = '',
        [string]$Directory,
        [System.Text.Encoding]$TextEncoding,
        [hashtable]$Environment = @{}
    )
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = $FileName
    $start.Arguments = $ArgumentLine
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    if ($Directory) { $start.WorkingDirectory = $Directory }
    if ($TextEncoding) {
        $start.StandardOutputEncoding = $TextEncoding
        $start.StandardErrorEncoding = $TextEncoding
    }
    foreach ($name in $Environment.Keys) {
        if ($null -eq $Environment[$name]) {
            $start.EnvironmentVariables.Remove($name)
        } else {
            $start.EnvironmentVariables[$name] = [string]$Environment[$name]
        }
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $start
    try {
        [void]$process.Start()
        # Drain both pipes concurrently. Sequential ReadToEnd calls can deadlock
        # when a compiler fills stderr while stdout is still being consumed.
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        return [pscustomobject]@{ ExitCode = $process.ExitCode; Output = $stdout; Error = $stderr }
    } finally {
        $process.Dispose()
    }
}

function Get-NativeToolPath {
    param([Parameter(Mandatory = $true)][string]$Name)
    $command = Get-Command -Name $Name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($command) { return $command.Source }
    return $null
}

function Initialize-MsvcEnvironment {
    param([string]$Installation)
    if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
        throw 'This build needs Windows with Visual Studio C++ tools and a Windows SDK.'
    }
    $requiredTools = @('cl.exe', 'link.exe', 'lib.exe')
    $alreadyReady = $env:VSCMD_ARG_TGT_ARCH -eq 'x86' -and $env:INCLUDE -and $env:LIB
    foreach ($tool in $requiredTools) {
        if (-not (Get-NativeToolPath $tool)) { $alreadyReady = $false }
    }
    if ($alreadyReady -and -not $Installation) {
        Write-Host 'Using the current MSVC x86 environment.'
        return
    }

    $roots = New-Object 'System.Collections.Generic.List[string]'
    if ($Installation) {
        [void]$roots.Add((Resolve-BuildPath $Installation))
    } else {
        $programRoots = @([Environment]::GetEnvironmentVariable('ProgramFiles(x86)'),
                          [Environment]::GetEnvironmentVariable('ProgramFiles')) |
            Where-Object { $_ } | Select-Object -Unique
        $vswhere = Get-NativeToolPath 'vswhere.exe'
        foreach ($programRoot in $programRoots) {
            $candidate = Join-Path $programRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
            if (-not $vswhere -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { $vswhere = $candidate }
        }
        if ($vswhere) {
            $found = Invoke-CapturedProcess -FileName $vswhere -TextEncoding ([Text.Encoding]::UTF8) `
                -ArgumentLine '-products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath -utf8'
            if ($found.ExitCode -eq 0) {
                foreach ($line in ($found.Output -split '\r?\n')) {
                    if ($line.Trim()) { [void]$roots.Add($line.Trim()) }
                }
            }
        }
        if ($env:VSINSTALLDIR) { [void]$roots.Add($env:VSINSTALLDIR) }
        # Covers the standard VS 2026/2022/2019 locations even if vswhere is
        # unavailable. A custom installation can be supplied with -VisualStudio.
        foreach ($version in @('18', '2022', '2019')) {
            foreach ($edition in @('Community', 'BuildTools', 'Professional', 'Enterprise')) {
                foreach ($programRoot in $programRoots) {
                    [void]$roots.Add((Join-Path $programRoot "Microsoft Visual Studio\$version\$edition"))
                }
            }
        }
    }
    $vcvars = $null
    foreach ($root in $roots) {
        $candidate = Join-Path $root 'VC\Auxiliary\Build\vcvarsall.bat'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { $vcvars = $candidate; break }
    }
    if (-not $vcvars) {
        throw 'Visual Studio C++ tools were not found. In Visual Studio Installer, install Desktop development with C++, MSVC x86/x64 tools and a Windows SDK. For a custom location, use -VisualStudio "C:\path\to\Visual Studio\Community".'
    }

    Write-Host "Initializing MSVC x86: $vcvars"
    $cmd = Join-Path ([Environment]::GetFolderPath('System')) 'cmd.exe'
    # cmd /u keeps the environment (including Unicode paths) in UTF-16. Only
    # import lines after our marker; vcvars banners are not environment values.
    $setup = Invoke-CapturedProcess -FileName $cmd -TextEncoding ([Text.Encoding]::Unicode) `
        -ArgumentLine '/d /u /v:off /s /c "call "%CODPORT_VCVARSALL%" x86 && echo __CODPORT_ENV_BEGIN__ && set"' `
        -Environment @{ CODPORT_VCVARSALL = $vcvars }
    $marker = '__CODPORT_ENV_BEGIN__'
    if ($setup.ExitCode -ne 0 -or -not $setup.Output.Contains($marker)) {
        $banner = ($setup.Output -split $marker)[0].Trim()
        throw "Visual Studio could not initialize x86 tools. Check the C++ workload and Windows SDK in Visual Studio Installer.`r`n$banner`r`n$($setup.Error)"
    }
    $import = $false
    foreach ($line in ($setup.Output -split '\r?\n')) {
        if ($line.Trim() -eq $marker) { $import = $true; continue }
        if (-not $import) { continue }
        $equals = $line.IndexOf('=')
        if ($equals -le 0) { continue } # Ignore cmd's =C: pseudo-variables.
        $name = $line.Substring(0, $equals)
        if ($name -ne 'CODPORT_VCVARSALL') {
            [Environment]::SetEnvironmentVariable($name, $line.Substring($equals + 1), 'Process')
        }
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne 'x86') {
        throw 'Visual Studio did not select the x86 target. Run this launcher from a new ordinary Command Prompt, or select the installation with -VisualStudio.'
    }
    foreach ($tool in $requiredTools) {
        if (-not (Get-NativeToolPath $tool)) {
            throw "Visual Studio initialization did not provide $tool. Install the MSVC x86/x64 C++ tools and Windows SDK."
        }
    }
}

function Copy-BuildSourceTree {
    param([string]$Source, [string]$Destination)
    [void][IO.Directory]::CreateDirectory($Destination)
    foreach ($entry in (Get-ChildItem -LiteralPath $Source -Force)) {
        if ($entry.Name -eq '.vs') { continue }
        $target = Join-Path $Destination $entry.Name
        if ($entry.PSIsContainer) {
            Copy-BuildSourceTree -Source $entry.FullName -Destination $target
        } else {
            [IO.File]::Copy($entry.FullName, $target, $true)
        }
    }
}

function Invoke-LoggedBuildTool {
    param([string]$Tool, [string[]]$Arguments, [string]$Directory, [string]$Label)
    $response = Join-Path $Directory ($Label + '.rsp')
    $log = Join-Path $Directory ($Label + '.log')
    $line = (@($Arguments | ForEach-Object { ConvertTo-WindowsArgument $_ }) -join ' ')
    [IO.File]::WriteAllText($response, $line, [Text.Encoding]::Unicode)
    Write-Host ((Split-Path -Leaf $Directory) + ': ' + $Label)
    $result = Invoke-CapturedProcess -FileName $Tool -ArgumentLine (ConvertTo-WindowsArgument ('@' + $response)) -Directory $Directory
    $messages = $result.Output
    if ($result.Error) { $messages += "`r`n[stderr]`r`n" + $result.Error }
    [IO.File]::WriteAllText($log, $messages, (New-Object Text.UTF8Encoding($false)))
    if ($result.ExitCode -ne 0) {
        foreach ($message in ($messages -split '\r?\n' | Select-Object -Last 20)) {
            [Console]::Error.WriteLine($message)
        }
        throw "$Tool failed with exit code $($result.ExitCode). Log: $log"
    }
}

function Invoke-WindowsBuild {
    param([string]$Root, [string]$Build, [string]$GamePgd, [string]$OpenAL)
    $Root = Resolve-BuildPath $Root
    $Build = Resolve-BuildPath $Build
    foreach ($supplied in @($GamePgd, $OpenAL)) {
        if ($supplied -and -not (Test-Path -LiteralPath $supplied -PathType Leaf)) {
            throw "File does not exist: $supplied"
        }
    }
    if ($GamePgd) { $GamePgd = Resolve-BuildPath $GamePgd }
    if ($OpenAL) { $OpenAL = Resolve-BuildPath $OpenAL }
    if (Test-Path -LiteralPath $Build) {
        if (-not (Test-Path -LiteralPath $Build -PathType Container) -or
            @(Get-ChildItem -LiteralPath $Build -Force).Count -gt 0) {
            throw "Choose an empty output directory: $Build (for example, -Output build_opt_r30_new). Existing files were kept."
        }
    }
    $sourceRoot = Join-Path $Root 'src'
    $sourcePrefix = $sourceRoot.TrimEnd([char[]]@([char]92, [char]47)) + [IO.Path]::DirectorySeparatorChar
    if ($Build.Equals($sourceRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $Build.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Choose an output directory outside src so source copying cannot include its own output.'
    }
    $targets = Get-Content -LiteralPath (Join-Path $Root 'tools/windows-x86.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    $headerSets = Get-Content -LiteralPath (Join-Path $Root 'tools/windows-headers.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    $runtime = Join-Path $Build 'bin'
    $inputs = Join-Path $Build 'obj/inputs'
    [void][IO.Directory]::CreateDirectory($runtime)
    Copy-BuildSourceTree -Source $sourceRoot -Destination (Join-Path $inputs 'src')
    $menuHeader = 'main/ui/menudef.h'
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent (Join-Path $inputs $menuHeader)))
    [IO.File]::Copy((Join-Path $Root $menuHeader), (Join-Path $inputs $menuHeader), $true)

    $activeHeaders = @{}
    $products = @{}
    # PSCustomObject properties preserve JSON declaration order. The executable
    # must link after its libraries; header state spans target boundaries too.
    foreach ($property in $targets.PSObject.Properties) {
        $name = $property.Name
        $target = $property.Value
        $objectDirectory = Join-Path $Build ('obj/' + $name)
        $dependencies = Join-Path $objectDirectory 'dependencies'
        [void][IO.Directory]::CreateDirectory($dependencies)
        $index = 0
        foreach ($group in $target.compile) {
            $headers = @{}
            if ($group.PSObject.Properties['headers']) {
                $set = $headerSets.PSObject.Properties[[string]$group.headers]
                if (-not $set) { throw "Unknown header set: $($group.headers)" }
                foreach ($header in $set.Value.PSObject.Properties) { $headers[$header.Name] = $header.Value }
            }
            foreach ($header in (@($activeHeaders.Keys) + @($headers.Keys) | Sort-Object -Unique)) {
                $revision = $header
                $previous = $header
                if ($headers.ContainsKey($header)) { $revision = $headers[$header] }
                if ($activeHeaders.ContainsKey($header)) { $previous = $activeHeaders[$header] }
                if ($revision -cne $previous) {
                    [IO.File]::Copy((Join-Path $Root $revision), (Join-Path $inputs $header), $true)
                }
            }
            $activeHeaders = $headers
            $compileArgs = New-Object 'System.Collections.Generic.List[string]'
            foreach ($flag in $group.flags) {
                if ($flag.StartsWith('/I')) { [void]$compileArgs.Add('/I' + (Join-Path $inputs $flag.Substring(2))) }
                else { [void]$compileArgs.Add($flag) }
            }
            [void]$compileArgs.Add('/MP2')
            [void]$compileArgs.Add('/sourceDependencies')
            [void]$compileArgs.Add($dependencies)
            foreach ($source in $group.sources) { [void]$compileArgs.Add((Join-Path $inputs $source)) }
            Invoke-LoggedBuildTool -Tool 'cl.exe' -Arguments $compileArgs.ToArray() -Directory $objectDirectory -Label "compile-$index"
            $index++
        }

        $library = $target.output.EndsWith('.lib', [StringComparison]::OrdinalIgnoreCase)
        $destination = $runtime
        if ($library) { $destination = $objectDirectory }
        $product = Join-Path $destination $target.output
        $linkArgs = New-Object 'System.Collections.Generic.List[string]'
        foreach ($flag in $target.link) {
            if ($flag -eq '@objects') {
                foreach ($source in $target.objects) {
                    [void]$linkArgs.Add((Join-Path $objectDirectory ([IO.Path]::GetFileNameWithoutExtension($source) + '.obj')))
                }
                continue
            }
            if ($flag.StartsWith('@')) {
                $dependency = $flag.Substring(1)
                if (-not $products.ContainsKey($dependency)) { throw "Target $name needs unavailable product $dependency." }
                $flag = $products[$dependency]
            } elseif ($flag.StartsWith('/DEF:')) {
                $flag = '/DEF:' + (Join-Path $Root $flag.Substring(5))
            }
            [void]$linkArgs.Add($flag)
        }
        if ($name -eq 'game' -and $GamePgd) {
            $profile = Join-Path $objectDirectory 'training.pgd'
            [IO.File]::Copy($GamePgd, $profile, $true)
            Write-Host ('GAME profile SHA-256 ' + (Get-FileHash -LiteralPath $profile -Algorithm SHA256).Hash.ToLowerInvariant())
            [void]$linkArgs.Add('/USEPROFILE:PGD=' + $profile)
        }
        [void]$linkArgs.Add('/OUT:' + $product)
        $linkTool = 'link.exe'
        if ($library) { $linkTool = 'lib.exe' }
        Invoke-LoggedBuildTool -Tool $linkTool -Arguments $linkArgs.ToArray() -Directory $objectDirectory -Label 'link'
        if (-not (Test-Path -LiteralPath $product -PathType Leaf)) { throw "Tool succeeded but its output is missing: $product" }
        $products[$name] = $product
    }

    $mainDirectory = Join-Path $Build 'profile/main'
    [void][IO.Directory]::CreateDirectory($mainDirectory)
    [IO.File]::Copy((Join-Path $Root 'main/training_player.cfg'), (Join-Path $mainDirectory 'training_player.cfg'), $true)
    [IO.File]::Copy((Join-Path $Root 'packaging/windows/wolfconfig.cfg'), (Join-Path $mainDirectory 'wolfconfig.cfg'), $true)
    foreach ($file in @('Play-Training.cmd', 'Play-Training-Single.cmd', 'Play-Training-Multi.cmd', 'Play-Training.ps1', 'THREADING-MODES.md')) {
        [IO.File]::Copy((Join-Path $Root ('packaging/windows/' + $file)), (Join-Path $Build $file), $true)
    }
    if ($OpenAL) { [IO.File]::Copy($OpenAL, (Join-Path $runtime 'soft_oal.dll'), $true) }
    Write-Host "Build complete.`r`nGame modules: $runtime`r`nTraining profile: $(Split-Path -Parent $mainDirectory)"
    if (-not $OpenAL) { Write-Host 'To run the game, put the 32-bit soft_oal.dll in bin, or supply -OpenAL when building.' }
}

# Dot-sourcing exposes the actual recipe runner to portable process/quoting tests
# without pretending that those tests compile Windows binaries.
if ($MyInvocation.InvocationName -ne '.') {
    $buildExitCode = 0
    try {
        $repository = Split-Path -Parent $PSScriptRoot
        if (-not $Output) { $Output = Join-Path $repository 'build' }
        Initialize-MsvcEnvironment -Installation $VisualStudio
        Invoke-WindowsBuild -Root $repository -Build $Output -GamePgd $GamePgd -OpenAL $OpenAL
    } catch {
        [Console]::Error.WriteLine('Build failed: ' + $_.Exception.Message)
        $buildExitCode = 1
    }
    exit $buildExitCode
}

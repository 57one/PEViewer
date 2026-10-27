#requires -Version 7.0
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('x86', 'Win32', 'x64')]
    [string]$Platform = 'x86',
    [switch]$All,
    [switch]$Rebuild,
    [switch]$Test,
    [string]$MSBuildPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $repoRoot 'PEViewer.vcxproj'

if ($MSBuildPath) {
    $MSBuildPath = (Resolve-Path -LiteralPath $MSBuildPath).Path
} else {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswherePath) {
        $MSBuildPath = & $vswherePath -latest -products '*' `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -find 'MSBuild\Current\Bin\MSBuild.exe' | Select-Object -First 1
    }
    if (!$MSBuildPath) {
        $command = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
        if ($command) { $MSBuildPath = $command.Source }
    }
    if (!$MSBuildPath) {
        throw 'MSBuild was not found. Install Visual Studio C++ tools or pass -MSBuildPath.'
    }
}

# Locate the developer environment belonging to this MSBuild installation.
$devCmdPath = $null
$candidateDirectory = Split-Path -Parent $MSBuildPath
while ($candidateDirectory) {
    $candidate = Join-Path $candidateDirectory 'Common7\Tools\VsDevCmd.bat'
    if (Test-Path -LiteralPath $candidate) {
        $devCmdPath = $candidate
        break
    }
    $candidateDirectory = Split-Path -Parent $candidateDirectory
}

function New-ProcessInfo {
    param([string]$FileName, [string[]]$Arguments, $Environment)
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $FileName
    $info.WorkingDirectory = $repoRoot
    $info.UseShellExecute = $false
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $info.Environment.Clear()
    foreach ($entry in $Environment.GetEnumerator()) {
        $info.Environment[$entry.Key] = $entry.Value
    }
    return $info
}

function Get-BuildEnvironment {
    param([string]$ProjectPlatform)
    $environment = [Collections.Generic.Dictionary[string, string]]::new(
        [StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
        $key = [string]$entry.Key
        $value = [string]$entry.Value
        if ($key -ieq 'PATH' -and $environment.ContainsKey($key)) {
            $value = $environment[$key] + ';' + $value
        }
        $environment[$key] = $value
    }
    if (!$devCmdPath) { return $environment }

    $architecture = if ($ProjectPlatform -eq 'Win32') { 'x86' } else { 'x64' }
    $hostArchitecture = if ([Environment]::Is64BitOperatingSystem) { 'x64' } else { 'x86' }
    $commandText = "chcp 65001 >nul && call `"$devCmdPath`" -no_logo -arch=$architecture -host_arch=$hostArchitecture >nul && set"
    $info = New-ProcessInfo $env:ComSpec @() $environment
    # cmd.exe expects its command text quoted as a whole, without C-runtime escaping.
    $info.Arguments = '/d /s /c "' + $commandText + '"'
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = [Text.Encoding]::UTF8
    $info.StandardErrorEncoding = [Text.Encoding]::UTF8
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "Visual Studio environment initialization failed: $($stderr.Result)"
        }
        foreach ($line in ($stdout.Result -split '\r?\n')) {
            if ($line -match '^([^=]+)=(.*)$') {
                $environment[$Matches[1]] = $Matches[2]
            }
        }
    } finally {
        $process.Dispose()
    }
    return $environment
}

function Invoke-MSBuild {
    param([string]$Project, [string]$Config, [string]$ProjectPlatform,
        [string]$LogDirectory, $Environment, [string[]]$ExtraProperties = @())
    New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
    $target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
    $hostArchitecture = if ([Environment]::Is64BitOperatingSystem) { 'x64' } else { 'x86' }
    $arguments = @($Project, "/t:$target", "/p:Configuration=$Config",
        "/p:Platform=$ProjectPlatform", "/p:PreferredToolArchitecture=$hostArchitecture",
        '/nologo', '/v:minimal', '/nr:false',
        "/flp:logfile=$LogDirectory\build.log;verbosity=normal;encoding=UTF-8") + $ExtraProperties
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = New-ProcessInfo $MSBuildPath $arguments $Environment
    try {
        [void]$process.Start()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "Build failed: $Project ($Config/$ProjectPlatform). See $LogDirectory\build.log"
        }
    } finally {
        $process.Dispose()
    }
}

$configurations = if ($All) { @('Debug', 'Release') } else { @($Configuration) }
$platforms = if ($All) { @('Win32', 'x64') } elseif ($Platform -eq 'x86') { @('Win32') } else { @($Platform) }
[xml]$mainProject = Get-Content -LiteralPath $projectPath -Raw
$namespaces = [Xml.XmlNamespaceManager]::new($mainProject.NameTable)
$namespaces.AddNamespace('p', $mainProject.DocumentElement.NamespaceURI)
$sdkNode = $mainProject.SelectSingleNode('//p:WindowsTargetPlatformVersion', $namespaces)
$environmentCache = @{}

foreach ($config in $configurations) {
    foreach ($projectPlatform in $platforms) {
        if (!$environmentCache.ContainsKey($projectPlatform)) {
            $environmentCache[$projectPlatform] = Get-BuildEnvironment $projectPlatform
        }
        $environment = $environmentCache[$projectPlatform]
        Write-Host "Building PEViewer: $config/$projectPlatform"
        $logDirectory = Join-Path $repoRoot "obj\$config\$projectPlatform"
        Invoke-MSBuild $projectPath $config $projectPlatform $logDirectory $environment

        if ($Test) {
            $configGroup = $mainProject.Project.PropertyGroup | Where-Object {
                $_.Label -eq 'Configuration' -and $_.Condition.Contains("'$config|$projectPlatform'")
            } | Select-Object -First 1
            if (!$configGroup) { throw "Configuration not found: $config/$projectPlatform" }
            $properties = @("/p:PlatformToolset=$($configGroup.PlatformToolset)")
            if ($sdkNode) { $properties += "/p:WindowsTargetPlatformVersion=$($sdkNode.InnerText)" }
            $testProject = Join-Path $repoRoot 'tests\portability-smoke.vcxproj'
            $testLogDirectory = Join-Path $logDirectory 'tests'
            Invoke-MSBuild $testProject $config $projectPlatform $testLogDirectory $environment $properties

            $testExecutable = Join-Path $repoRoot "bin\$config\$projectPlatform\tests\portability-smoke.exe"
            $fixturePath = Join-Path $testLogDirectory 'synthetic-pe32.exe'
            $process = [Diagnostics.Process]::new()
            $process.StartInfo = New-ProcessInfo $testExecutable @($fixturePath) $environment
            try {
                [void]$process.Start()
                # Full UI regression covers both formats and repeated resize/paint cycles.
                if (!$process.WaitForExit(180000)) {
                    $process.Kill($true)
                    $process.WaitForExit()
                    throw "Smoke test timed out: $config/$projectPlatform"
                }
                if ($process.ExitCode -ne 0) { throw "Smoke test failed: $config/$projectPlatform" }
            } finally {
                $process.Dispose()
            }
        }
    }
}
Write-Host 'All requested builds and checks passed.'

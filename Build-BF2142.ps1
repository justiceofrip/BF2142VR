[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('x86', 'x64')]
    [string]$Architecture,
    [ValidateRange(1, 16)]
    [int]$Jobs = 4,
    [ValidatePattern('^[a-zA-Z0-9_-]*$')]
    [string]$Variant = ''
)
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$workspaceRoot = $PSScriptRoot
$sourceRoot = $workspaceRoot
$buildLabel = "bf2142-$Architecture"
if ($Variant) { $buildLabel += "-$Variant" }
$buildRoot = Join-Path $workspaceRoot "build\$buildLabel"
$logsRoot = Join-Path $workspaceRoot 'logs'
New-Item -ItemType Directory -Path $logsRoot -Force | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) { throw 'A Visual Studio C++ installation is required.' }
$devShell = Join-Path $vsInstall 'Common7\Tools\Launch-VsDevShell.ps1'
& $devShell -VsInstallationPath $vsInstall -Arch $Architecture -HostArch amd64 -SkipAutomaticLocation | Out-Null
if (-not $env:WindowsSdkDir) { throw 'The Windows SDK is not available in the developer environment.' }
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
$generator = if ($ninja) { 'Ninja' } else { 'NMake Makefiles' }

function Invoke-LoggedBuildStep {
    param([string]$Name, [string]$Executable, [string[]]$Arguments)
    $logPath = Join-Path $logsRoot "$buildLabel-$Name.log"
    Write-Output "$Architecture $Name started; log: $logPath"
    # Windows PowerShell 5 treats native stderr warnings as error records.
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Executable @Arguments *> $logPath
        $stepExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($stepExitCode -ne 0) {
        Get-Content -LiteralPath $logPath -Tail 65
        throw "$Name failed with exit code $stepExitCode."
    }
    Get-Content -LiteralPath $logPath -Tail 5
}

$configArguments = @('-S', (Join-Path $sourceRoot 'src'), '-B', $buildRoot, '-G', $generator, '-DCMAKE_BUILD_TYPE=RelWithDebInfo')
if ($Architecture -eq 'x64') { $configArguments += '-DBFVR_PRESENTER_ONLY=ON' }
if ($Architecture -eq 'x86') { $configArguments += '-DBFVR_BUILD_BF2142_BOOTSTRAP=ON' }
Invoke-LoggedBuildStep 'configure' $cmake $configArguments
$buildArguments = @('--build', $buildRoot, '--parallel', "$Jobs")
if ($Architecture -eq 'x64') { $buildArguments += @('--target', 'BFVRPresenter') }
Invoke-LoggedBuildStep 'build' $cmake $buildArguments
if ($Architecture -eq 'x86') {
    Invoke-LoggedBuildStep 'tests' $ctest @('--test-dir', $buildRoot, '--output-on-failure')
}
$revision = 'source-snapshot'
if (Test-Path -LiteralPath (Join-Path $sourceRoot '.git')) {
    $candidateRevision = & git -C $sourceRoot rev-parse --verify HEAD 2>$null
    if ($LASTEXITCODE -eq 0) { $revision = $candidateRevision }
}
[ordered]@{
    architecture = $Architecture
    source_revision = $revision
    completed_utc = [DateTime]::UtcNow.ToString('o')
    build_directory = $buildRoot
    compiler_installation = $vsInstall
    sdk_version = $env:WindowsSDKVersion
    generator = $generator
    build_passed = $true
    win32_ctest_passed = ($Architecture -eq 'x86')
    headset_tested = $false
    bf2142_integration_tested = $false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logsRoot "$buildLabel-result.json") -Encoding utf8
Write-Output "$buildLabel build completed."


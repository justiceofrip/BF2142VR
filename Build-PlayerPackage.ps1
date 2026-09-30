[CmdletBinding()]
param(
    [string]$X86Build = '',
    [string]$X64Build = '',
    [string]$Destination = '',
    [string]$Python = ''
)
$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
if (-not $X86Build) { $X86Build = Join-Path $repo 'build\bf2142-x86' }
if (-not $X64Build) { $X64Build = Join-Path $repo 'build\bf2142-x64' }
if (-not $Destination) { $Destination = Join-Path $repo 'build\player-candidate\BF2142 VR Beta' }
if (Test-Path -LiteralPath $Destination) { throw 'Choose a new destination; candidates are never overwritten.' }
$packageSource = Join-Path $repo 'scripts\bf2142\package'
$assetsSource = Join-Path $repo 'scripts\bf2142'
$work = Join-Path $repo ('build\package-tools-' + [guid]::NewGuid().ToString('N'))
$venv = Join-Path $work 'python'
$checkpoint = Join-Path $work 'checkpoint'
foreach ($file in @((Join-Path $X86Build 'bf2142\BF2142VRLauncher.exe'), (Join-Path $X86Build 'bf2142\BF2142VRClient.dll'), (Join-Path $X64Build 'BFVRPresenter.exe'))) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Build both architectures first. Missing: $file" }
}
if (-not $Python) {
    $Python = & py -3.13 -c 'import sys; print(sys.executable)'
    if ($LASTEXITCODE -ne 0) { throw 'Install Python 3.13.15 or supply -Python with its executable path.' }
}
$version = & $Python -c 'import platform; print(platform.python_version())'
if ($LASTEXITCODE -ne 0 -or $version -ne '3.13.15') {
    throw 'The beta packaging license set is pinned to Python 3.13.15. Supply that interpreter; review component versions/licenses before changing it.'
}
function Invoke-Native {
    param([string]$Executable, [string[]]$Arguments)
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Executable @Arguments
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($code -ne 0) { throw "Command failed ($code): $Executable" }
}
New-Item -ItemType Directory -Path $work -Force | Out-Null
Invoke-Native $Python @('-m','venv',$venv)
$pythonExe = Join-Path $venv 'Scripts\python.exe'
Invoke-Native $pythonExe @('-m','pip','install','-r',(Join-Path $packageSource 'requirements-build.txt'))
Invoke-Native $pythonExe @('-m','PyInstaller','--noconfirm','--clean','--onedir','--console','--noupx','--name','SetupAssets','--paths',$assetsSource,'--hidden-import','PIL.DdsImagePlugin','--hidden-import','PIL.PngImagePlugin','--hidden-import','PIL.IcoImagePlugin','--exclude-module','tkinter','--exclude-module','numpy','--distpath',(Join-Path $work 'dist'),'--workpath',(Join-Path $work 'pyinstaller'),'--specpath',$work,(Join-Path $packageSource 'SetupAssets.py'))
New-Item -ItemType Directory -Path (Join-Path $checkpoint 'x86'),(Join-Path $checkpoint 'x64') -Force | Out-Null
foreach ($name in @('BF2142VRLauncher.exe','BF2142VRClient.dll')) {
    Copy-Item -LiteralPath (Join-Path $X86Build "bf2142\$name") -Destination (Join-Path $checkpoint "x86\$name")
}
Copy-Item -LiteralPath (Join-Path $X64Build 'BFVRPresenter.exe') -Destination (Join-Path $checkpoint 'x64\BFVRPresenter.exe')
foreach ($name in @('assets','runtime')) {
    Copy-Item -LiteralPath (Join-Path $X64Build $name) -Destination (Join-Path $checkpoint "x64\$name") -Recurse
}
Invoke-Native $pythonExe @((Join-Path $packageSource 'StagePlayer.py'),'--checkpoint',$checkpoint,'--tools',(Join-Path $work 'dist\SetupAssets'),'--python-home',(Split-Path -Parent $Python),'--python-env',$venv,'--destination',$Destination)
Write-Output "Candidate staged: $Destination"
Write-Output 'Complete the package release checklist before publishing; this helper does not create a public release.'

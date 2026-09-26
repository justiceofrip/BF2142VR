param([string]$StateDir='C:\ProgramData\BF2142VR\Host')
$ErrorActionPreference='Stop'
$bundle=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
& (Join-Path $bundle 'BF2142Community.exe') host-stop --config (Join-Path $StateDir 'host.json')
exit $LASTEXITCODE

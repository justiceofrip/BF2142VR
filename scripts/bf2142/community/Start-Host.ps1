[CmdletBinding()]
param([string]$StateDir='C:\ProgramData\BF2142VR\Host')
$ErrorActionPreference='Stop'
$bundle=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$helper=Join-Path $bundle 'BF2142Community.exe'
$config=Join-Path $StateDir 'host.json'
if(!(Test-Path -LiteralPath $config)){throw 'Run Prepare-Host.ps1 first.'}
if(Get-Process BF2142_w32ded -ErrorAction SilentlyContinue){throw 'A dedicated server is already running; no second instance started.'}
& $helper host-run --config $config
exit $LASTEXITCODE

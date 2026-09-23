[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Manifest,[Parameter(Mandatory=$true)][string]$Presenter,[Parameter(Mandatory=$true)][string]$Log)
# Optional metadata registration must never delay or prevent the game launch.
try {
 $register=Join-Path $PSScriptRoot 'Register-SteamVR.ps1'
 $powershell=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
 $brandingArguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$register+'"'),'-Manifest',('"'+[IO.Path]::GetFullPath($Manifest)+'"'),'-Presenter',('"'+[IO.Path]::GetFullPath($Presenter)+'"'),'-Watch','-Log',('"'+[IO.Path]::GetFullPath($Log)+'"'))
 Start-Process -FilePath $powershell -ArgumentList $brandingArguments -WindowStyle Hidden | Out-Null
} catch {Write-Warning ('SteamVR branding helper unavailable: '+$_.Exception.Message)}

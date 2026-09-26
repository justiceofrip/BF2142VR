[CmdletBinding()]
param([Parameter(Mandatory)][string]$ServerDir,[Parameter(Mandatory)][string]$PublicAddress,
 [string]$StateDir='C:\ProgramData\BF2142VR\Host',[string]$Build='v35-community',[switch]$LocalOnly)
$ErrorActionPreference='Stop'
$bundle=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$helper=Join-Path $bundle 'BF2142Community.exe'
$runtime=Join-Path $bundle 'runtime\server'
$ServerDir=(Resolve-Path -LiteralPath $ServerDir).Path
$StateDir=[IO.Path]::GetFullPath($StateDir)
if(Test-Path -LiteralPath $StateDir){throw 'Choose a new StateDir; existing host settings are not overwritten.'}
if(Get-Process BF2142_w32ded -ErrorAction SilentlyContinue){throw 'Close the dedicated server before preparing it.'}
$admin=Join-Path $ServerDir 'admin\default.py'
$original=[IO.File]::ReadAllText($admin)
$pattern="self\.sock\.bind\(\(\s*['"" ]*(?:0\.0\.0\.0)?['""]\s*,\s*self\.port\s*\)\)"
$loopback="self.sock.bind(('127.0.0.1', self.port))"
if($original.Contains($loopback)){$patched=$original}
elseif(([regex]::Matches($original,$pattern)).Count -eq 1){$patched=[regex]::Replace($original,$pattern,$loopback)}
else{throw 'Unrecognized RCON bind code. Keep it private and review the native admin module before deploying.'}
$args=@('init-host','--root',$StateDir,'--game',$ServerDir,'--runtime',$runtime,'--address',$PublicAddress,'--build',$Build)
if($LocalOnly){$args+=@('--local','1')}
& $helper @args
if($LASTEXITCODE -ne 0){throw 'Host configuration could not be prepared.'}
# Secrets remain readable only to this Windows account, administrators and SYSTEM.
$sid=[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
& icacls.exe $StateDir /inheritance:r /grant:r "*$($sid):(OI)(CI)F" '*S-1-5-18:(OI)(CI)F' '*S-1-5-32-544:(OI)(CI)F' | Out-Null
if($LASTEXITCODE -ne 0){throw 'Could not restrict the host secrets directory.'}
$c=Get-Content -LiteralPath (Join-Path $StateDir 'host.json') -Raw|ConvertFrom-Json
$backups=Join-Path $StateDir 'backups';[IO.Directory]::CreateDirectory($backups)|Out-Null
Copy-Item -LiteralPath $admin -Destination (Join-Path $backups 'default.py')
$cfg=Join-Path $ServerDir 'admin\default.cfg'
if(Test-Path -LiteralPath $cfg){Copy-Item -LiteralPath $cfg -Destination (Join-Path $backups 'default.cfg')}
[IO.File]::WriteAllText($admin,$patched,[Text.Encoding]::ASCII)
[IO.File]::WriteAllText($cfg,('port='+$c.RconPort+"`r`npassword="+$c.RconPassword+"`r`n"),[Text.Encoding]::ASCII)
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'bfvr_community.py') -Destination (Join-Path $ServerDir 'python\bfvr_community.py')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'bfvr_community_admin.py') -Destination (Join-Path $ServerDir 'admin\bfvr_community_admin.py')
$stock=Join-Path $ServerDir 'mods\bf2142\std_archive.md5'
if(Test-Path -LiteralPath $stock){
 $resolved=(Resolve-Path -LiteralPath $stock).Path
 if(!$resolved.StartsWith($ServerDir+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Checksum path escaped the server directory.'}
 Move-Item -LiteralPath $resolved -Destination (Join-Path $backups 'std_archive.md5')
}
$env:BF2142VR_NETWORK=$c.NetworkConfig
& (Join-Path $runtime 'BF2142VRServerLauncher.exe') $ServerDir $c.ServerSettings $c.MapList --check
if($LASTEXITCODE -ne 0){throw 'The server executable does not pass the addon compatibility check.'}
Write-Output "Host prepared. Review $($c.ServerSettings), then run Start-Host.ps1 -StateDir `"$StateDir`"."

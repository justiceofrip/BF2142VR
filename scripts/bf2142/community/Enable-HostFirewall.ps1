# Run on the chosen cloud server, after reviewing its host configuration.
[CmdletBinding()]
param([string]$StateDir='C:\ProgramData\BF2142VR\Host')
$ErrorActionPreference='Stop'
$c=Get-Content -LiteralPath (Join-Path $StateDir 'host.json') -Raw|ConvertFrom-Json
$rules=@(
 @{Name='BF2142VR-Game';Protocol='UDP';Port=17567;Program=(Join-Path $c.GameDirectory 'BF2142_w32ded.exe')},
 @{Name='BF2142VR-Query';Protocol='UDP';Port=29900;Program=(Join-Path $c.GameDirectory 'BF2142_w32ded.exe')},
 @{Name='BF2142VR-NativeVoice';Protocol='UDP';Port=55124;Program=(Join-Path $c.GameDirectory 'BF2142_w32ded.exe')},
 @{Name='BF2142VR-TLS';Protocol='TCP';Port=$c.ControlPort},
 @{Name='BF2142VR-EncryptedUDP';Protocol='UDP';Port=$c.UdpPort}
)
foreach($r in $rules){if(Get-NetFirewallRule -Name $r.Name -ErrorAction SilentlyContinue){continue};$args=@{Name=$r.Name;DisplayName=$r.Name;Direction='Inbound';Action='Allow';Protocol=$r.Protocol;LocalPort=$r.Port;Profile='Any'};if($r.Program){$args.Program=$r.Program};New-NetFirewallRule @args | Out-Null}
Write-Output 'Game, query, native voice and encrypted addon ports allowed. RCON and native pose/proximity ports remain private. Add the same five ports in the cloud firewall.'

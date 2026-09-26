[CmdletBinding()]
param([ValidateSet('List','Change','Next')][string]$Action='List',
 [string]$Map,[string]$StateDir='C:\ProgramData\BF2142VR\Host')
$ErrorActionPreference='Stop'
$c=Get-Content -LiteralPath (Join-Path $StateDir 'host.json') -Raw | ConvertFrom-Json
$client=New-Object Net.Sockets.TcpClient
try {
 $client.Connect('127.0.0.1',[int]$c.RconPort)
 $stream=$client.GetStream();$stream.ReadTimeout=5000;$stream.WriteTimeout=5000
 function Read-Reply([string]$end){
  $text=New-Object Text.StringBuilder
  while($text.Length -lt 65536){
   $value=$stream.ReadByte();if($value -lt 0){throw 'Native server closed the management connection.'}
   [void]$text.Append([char]$value)
   if($text.ToString().EndsWith($end)){return $text.ToString().Substring(0,$text.Length-$end.Length)}
  }
  throw 'Native management reply exceeded its limit.'
 }
 function Send-Command([string]$command){
  if($command.Contains("`n") -or $command.Contains("`r")){throw 'Invalid command'}
  $bytes=[Text.Encoding]::ASCII.GetBytes(([char]2)+$command+"`n")
  $stream.Write($bytes,0,$bytes.Length)
  $reply=Read-Reply ([string][char]4)
  if($reply -match 'Unknown object or method|not found|syntax error'){throw $reply}
  return $reply
 }
 $hello=Read-Reply "`n`n"
 $seed=[regex]::Match($hello,'Digest seed: (\w+)').Groups[1].Value
 if(!$seed){throw 'Unexpected native management greeting'}
 $md=[Security.Cryptography.MD5]::Create()
 try{$digest=([BitConverter]::ToString($md.ComputeHash([Text.Encoding]::ASCII.GetBytes($seed+$c.RconPassword)))).Replace('-','').ToLowerInvariant()}finally{$md.Dispose()}
 if((Send-Command ('login '+$digest)) -notmatch 'Authentication successful'){throw 'Local management authentication failed'}
 $rotation=Send-Command 'exec mapList.list'
 if($Action -eq 'List'){$rotation;return}
 if($Action -eq 'Change'){
  if($Map -notmatch '^[A-Za-z0-9_]+$'){throw 'Choose a map name from List'}
  $entry=[regex]::Match($rotation,'(?mi)^\s*(\d+):\s*"'+[regex]::Escape($Map)+'"\s+gpm_(?:coop|cq|ca|sl|ti)\s+(?:16|32|48|64)\s*$')
  if(!$entry.Success){throw 'That map is not in the current rotation. Run List.'}
  Send-Command ('exec admin.nextLevel '+$entry.Groups[1].Value)
 }
 Send-Command 'exec admin.runNextLevel'
 Write-Output 'Map change requested. Connected players will load the selected map.'
}finally{$client.Close()}

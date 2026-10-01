[CmdletBinding()]
param(
 [ValidateSet('Reference','NoFXAA','NoBloom','Clean')][string]$Mode='Clean',
 [string]$InstallRoot='',
 [switch]$PrepareOnly,
 [string]$EvidenceDirectory=''
)
$ErrorActionPreference='Stop'
# Each run gets a private settings snapshot. Never edit the installed config,
# resolution, native AA, weapon timing, or graphics driver settings.
if(-not $InstallRoot){
 if(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'install.json')){$InstallRoot=$PSScriptRoot}
 else{
  Add-Type -AssemblyName System.Windows.Forms
  $picker=New-Object System.Windows.Forms.OpenFileDialog
  $picker.Title='Select your installed Battlefield 2142 game'
  $picker.Filter='Battlefield 2142 (BF2142.exe)|BF2142.exe'
  try{
   if($picker.ShowDialog() -ne [Windows.Forms.DialogResult]::OK){return}
   $InstallRoot=Join-Path ([IO.Path]::GetDirectoryName($picker.FileName)) 'BF2142VR'
  }finally{$picker.Dispose()}
 }
}
$root=(Resolve-Path -LiteralPath $InstallRoot).ProviderPath
$player=Join-Path $root 'tools\Player.ps1'
$metadata=Join-Path $root 'install.json'
if(-not (Test-Path -LiteralPath $player -PathType Leaf) -or -not (Test-Path -LiteralPath $metadata -PathType Leaf)){
 throw 'Select a game with BF2142 VR already installed. This comparison tool does not install the mod.'
}
if((Get-Content -LiteralPath $metadata -Raw|ConvertFrom-Json).status -ne 'installed'){throw 'The selected VR installation is not ready.'}
$originalEnvironment=[Environment]::GetEnvironmentVariable('BFVR_USER_CONFIG_PATH','Process')
$source=if($originalEnvironment){$originalEnvironment}else{Join-Path $root 'runtime\x64\UserConfig.txt'}
if(-not $originalEnvironment -and (Test-Path -LiteralPath (Join-Path (Get-Location).Path 'BFVR') -PathType Container)){
 $source=Join-Path (Get-Location).Path 'BFVR\UserConfig.txt'
}
$text=if(Test-Path -LiteralPath $source -PathType Leaf){[IO.File]::ReadAllText($source)}else{"# Inherit normal presenter defaults.`r`nschema_version = 1`r`n"}
if([regex]::Matches($text,'(?m)^[\t ]*schema_version[\t ]*=[\t ]*1[\t ]*\r?$').Count -ne 1){
 throw 'Presenter settings schema is missing or unsupported; comparison cancelled.'
}
$overrides=@{}
if($Mode -in @('NoFXAA','Clean')){$overrides['fxaa_enabled']='false'}
if($Mode -in @('NoBloom','Clean')){$overrides['bloom_enabled']='false'}
foreach($key in $overrides.Keys){
 $expression='(?m)^[\t ]*'+[regex]::Escape($key)+'[\t ]*=[^\r\n]*'
 $foundKeys=[regex]::Matches($text,$expression)
 if($foundKeys.Count -gt 1){throw "Duplicate setting $key in presenter config; comparison cancelled."}
 if($foundKeys.Count){$text=[regex]::Replace($text,$expression,$key+' = '+$overrides[$key])}
 else{$text+="`r`n"+$key+' = '+$overrides[$key]+"`r`n"}
}
if(-not $EvidenceDirectory){
 $EvidenceDirectory=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) ('BF2142VR\ImageComparison\'+[guid]::NewGuid().ToString('N'))
}
if(Test-Path -LiteralPath $EvidenceDirectory){throw 'Choose a new evidence directory; previous comparisons are retained.'}
$evidence=(New-Item -ItemType Directory -Path $EvidenceDirectory -ErrorAction Stop).FullName
$config=Join-Path $evidence 'UserConfig.txt'
[IO.File]::WriteAllText($config,$text,(New-Object Text.UTF8Encoding($false)))
$sourceHash=if(Test-Path -LiteralPath $source -PathType Leaf){(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash}else{'normal-defaults'}
$report=[pscustomobject]@{mode=$Mode;config=$config;source=$source;sourceSha256=$sourceHash;overrides=$overrides;player=$player}
$report|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $evidence 'comparison.json') -Encoding UTF8
Write-Host "Image comparison: $Mode. Temporary presenter settings: $config"
Write-Host 'Resolution and native MSAA remain unchanged. Normal Play VR restores your usual settings.'
if($PrepareOnly){return $report}
if(Get-Process -Name BF2142 -ErrorAction SilentlyContinue){throw 'Close BF2142 normally before launching a comparison.'}
try{
 [Environment]::SetEnvironmentVariable('BFVR_USER_CONFIG_PATH',$config,'Process')
 & $player -Action Play
 if($LASTEXITCODE -ne 0){throw "VR launcher exited with code $LASTEXITCODE. Check runtime\x86\logs."}
}finally{
 [Environment]::SetEnvironmentVariable('BFVR_USER_CONFIG_PATH',$originalEnvironment,'Process')
}

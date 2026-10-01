$ErrorActionPreference='Stop'
$base=Join-Path ([IO.Path]::GetTempPath()) ('BF2142FilterTest-'+[guid]::NewGuid().ToString('N'))
$root=Join-Path $base 'Game\BF2142VR'
[void](New-Item -ItemType Directory -Path (Join-Path $root 'runtime\x64'),(Join-Path $root 'tools') -Force)
'{"status":"installed"}'|Set-Content -LiteralPath (Join-Path $root 'install.json')
"throw 'PrepareOnly must never launch the game'"|Set-Content -LiteralPath (Join-Path $root 'tools\Player.ps1')
$source=Join-Path $root 'runtime\x64\UserConfig.txt'
$initial="# test fixture`r`nschema_version = 1`r`nfxaa_enabled = true`r`nbloom_enabled = true`r`nfxaa_sharpening_percent = 65`r`ncolor_contrast_percent = 10`r`n"
[IO.File]::WriteAllText($source,$initial)
$saved=[Environment]::GetEnvironmentVariable('BFVR_USER_CONFIG_PATH','Process')
try{
 [Environment]::SetEnvironmentVariable('BFVR_USER_CONFIG_PATH',$source,'Process')
 foreach($mode in @('Reference','NoFXAA','NoBloom','Clean')){
  $result=& (Join-Path $PSScriptRoot 'Compare-ImageFilters.ps1') -Mode $mode -InstallRoot $root -PrepareOnly -EvidenceDirectory (Join-Path $base $mode)
  $text=[IO.File]::ReadAllText($result.config)
  $fxaa=if($mode -in @('NoFXAA','Clean')){'false'}else{'true'}
  $bloom=if($mode -in @('NoBloom','Clean')){'false'}else{'true'}
  if(-not $text.Contains('fxaa_enabled = '+$fxaa) -or -not $text.Contains('bloom_enabled = '+$bloom)){throw 'Wrong isolated overrides'}
  if(-not $text.Contains('fxaa_sharpening_percent = 65') -or -not $text.Contains('color_contrast_percent = 10')){throw 'Unrelated setting changed'}
  if([IO.File]::ReadAllText($source) -cne $initial){throw 'Original configuration modified'}
  if([Environment]::GetEnvironmentVariable('BFVR_USER_CONFIG_PATH','Process') -cne $source){throw 'Environment changed by preparation'}
 }
 [IO.File]::WriteAllText($source,"schema_version = 1`r`nfxaa_enabled = true`r`nfxaa_enabled = false`r`n")
 $failed=$false
 try{& (Join-Path $PSScriptRoot 'Compare-ImageFilters.ps1') -Mode NoFXAA -InstallRoot $root -PrepareOnly -EvidenceDirectory (Join-Path $base 'duplicate')}catch{$failed=$true}
 if(-not $failed){throw 'Duplicate configuration was accepted'}
 [IO.File]::WriteAllText($source,"# no saved AA keys yet`r`nschema_version = 1`r`n")
 $result=& (Join-Path $PSScriptRoot 'Compare-ImageFilters.ps1') -Mode Clean -InstallRoot $root -PrepareOnly -EvidenceDirectory (Join-Path $base 'missing-keys')
 if(-not [IO.File]::ReadAllText($result.config).Contains('bloom_enabled = false')){throw 'Missing keys not added'}
 Write-Host 'PASS: four isolated filter modes, originals/environment preserved, no game launched, duplicate rejection and missing-key support.'
}finally{[Environment]::SetEnvironmentVariable('BFVR_USER_CONFIG_PATH',$saved,'Process')}

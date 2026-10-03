[CmdletBinding()]
param([ValidateSet('Setup','Play','Desktop','Uninstall','Inspect')][string]$Action='Setup',[string]$GameDir,[switch]$NoShortcut,[switch]$NonInteractive)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$worker=Join-Path $PSScriptRoot 'SetupAssets.exe'
function Run-Worker([string[]]$Arguments){
 # Native stderr must not become a terminating PowerShell exception that hides
 # the actual worker traceback or bypasses its exit-code check.
 $logs=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'BF2142VR\SetupLogs'
 [void][IO.Directory]::CreateDirectory($logs)
 $log=Join-Path $logs ('setup-'+[DateTime]::Now.ToString('yyyyMMdd-HHmmss-fff')+'.log')
 Write-Host "Setup log: $log"
 $savedPreference=$ErrorActionPreference
 try {
  $ErrorActionPreference='Continue'
  & $worker @Arguments 2>&1 | ForEach-Object {
   $line=$_.ToString();Write-Host $line
   Add-Content -LiteralPath $log -Value $line -Encoding UTF8
  }
  $code=$LASTEXITCODE
 } finally {$ErrorActionPreference=$savedPreference}
 if($code -ne 0){
  $hex='{0:X8}' -f ([long]$code -band 0xffffffffL)
  Add-Content -LiteralPath $log -Value "Worker exit: $code (0x$hex)" -Encoding UTF8
  throw "Setup stopped (exit $code / 0x$hex). Details: $log. Original backups are retained if setup reached the backup stage."
 }
}
function Pick-Game {
 Add-Type -AssemblyName System.Windows.Forms
 $picker=New-Object System.Windows.Forms.OpenFileDialog
 $picker.Title='Select BF2142.exe in your installed Battlefield 2142 folder'
 $picker.Filter='Battlefield 2142 (BF2142.exe)|BF2142.exe'
 $picker.CheckFileExists=$true
 try {if($picker.ShowDialog() -ne 'OK'){return $null};return [IO.Path]::GetDirectoryName($picker.FileName)} finally {$picker.Dispose()}
}
try {
 if($Action -eq 'Setup'){
  if(-not $GameDir){$GameDir=Pick-Game}
  if(-not $GameDir){Write-Host 'Setup cancelled.';exit 0}
  $GameDir=(Resolve-Path -LiteralPath $GameDir).Path
  Write-Host 'Building Battlefield 2142 VR from your installed game. This can take a few minutes.'
  Run-Worker @('apply','--game',$GameDir,'--payload',$root)
  $installed=Join-Path $GameDir 'BF2142VR'
  if(-not $NoShortcut){
   try {
    $shell=New-Object -ComObject WScript.Shell
    $linkPath=Join-Path ([Environment]::GetFolderPath('Desktop')) 'Battlefield 2142 VR.lnk'
    $link=$shell.CreateShortcut($linkPath)
    $launcher=Join-Path $installed 'BF2142VRSetup.exe'
    $cache=Join-Path ([Environment]::GetFolderPath('LocalApplicationData')) 'BF2142VR\Updater'
    [void][IO.Directory]::CreateDirectory($cache)
    $stream=[IO.File]::OpenRead($launcher);$hasher=[Security.Cryptography.SHA256]::Create()
    try {$hash=([BitConverter]::ToString($hasher.ComputeHash($stream))).Replace('-','')} finally {$stream.Dispose();$hasher.Dispose()}
    $cached=Join-Path $cache ('BF2142VR-'+$hash.Substring(0,16)+'.exe')
    if(-not [IO.File]::Exists($cached)){[IO.File]::Copy($launcher,$cached)}
    else {
     $stream=[IO.File]::OpenRead($cached);$hasher=[Security.Cryptography.SHA256]::Create()
     try {$cacheHash=([BitConverter]::ToString($hasher.ComputeHash($stream))).Replace('-','')} finally {$stream.Dispose();$hasher.Dispose()}
     if($cacheHash -ne $hash){throw 'Cached VR launcher was modified. Use the downloaded launcher to repair it.'}
    }

    $launcher=$cached

    # Start once to retain a versioned launcher outside the install directory.
    # Subsequent updates can replace the runtime without locking this UI.
    $link.TargetPath=$launcher;$link.Arguments='--game "'+$GameDir+'"';$link.WorkingDirectory=$GameDir;$link.Description='Battlefield 2142 VR launcher and updater';$link.Save()

   } catch {Write-Warning 'Could not create the desktop shortcut. Use Play VR.cmd in the game''s BF2142VR folder.'}
  }
  Write-Host "Ready: $installed\Play VR.cmd"
  Write-Host 'Connect your headset, start SteamVR, then launch Battlefield 2142 VR Beta.'
  exit 0
 }
 if(-not(Test-Path -LiteralPath (Join-Path $root 'install.json'))){
  if(-not $GameDir){$GameDir=Pick-Game}
  if(-not $GameDir){exit 0}
  $installed=Join-Path $GameDir 'BF2142VR'
  if(-not(Test-Path -LiteralPath (Join-Path $installed 'install.json'))){throw 'Run Setup.cmd first.'}
  & (Join-Path $installed 'tools\Player.ps1') -Action $Action
  exit $LASTEXITCODE
 }
 if($Action -eq 'Uninstall'){
  Run-Worker @('uninstall','--root',$root)
  $linkPath=Join-Path ([Environment]::GetFolderPath('Desktop')) 'Battlefield 2142 VR Beta.lnk'
  if(Test-Path -LiteralPath $linkPath){
   $shell=New-Object -ComObject WScript.Shell;$link=$shell.CreateShortcut($linkPath)
   $expected='-NoProfile -ExecutionPolicy Bypass -File "'+(Join-Path $root 'tools\Player.ps1')+'" -Action Play'
   if($link.Arguments -eq $expected){Remove-Item -LiteralPath $linkPath}
  }
  exit 0
 }
 Run-Worker @('check','--root',$root)
 $game=(Get-Content -LiteralPath (Join-Path $root 'install.json') -Raw|ConvertFrom-Json).game
 $env:BF2142VR_CONFIG=Join-Path $root 'BF2142VR.ini'
 $env:BFVR_DIAGNOSTICS='off'
 $env:BF2142VR_FRAME_PROFILE='0'
 $env:BF2142VR_GPU_DEBUG='0'
 $env:BF2142VR_MENU_TRACE='0'
 if($Action -ne 'Desktop'){
  $env:BF2142VR_GPU_TRANSFER='dx9ex'
  $env:BF2142VR_EX_MANAGED_UPLOAD='1'
 }
 $env:BFVR_USER_CONFIG_PATH=Join-Path $root 'runtime\x64\UserConfig.txt'
 $launcher=Join-Path $root 'runtime\x86\BF2142VRLauncher.exe'
 $presenter=Join-Path $root 'runtime\x64\BFVRPresenter.exe'
 $launchArgs=@('--game-dir',$game,'--windowed')
 if($Action -eq 'Desktop'){$launchArgs+='--desktop-vr'}else{$launchArgs+=@('--presenter',$presenter,'--headset-resolution')}
 if($Action -eq 'Inspect'){$launchArgs+='--inspect'}
 elseif($Action -ne 'Desktop'){
  $manifest=Join-Path $root 'BF2142VR.vrmanifest'
  if(Test-Path -LiteralPath $manifest){
   $logs=Join-Path $root 'logs';[void][IO.Directory]::CreateDirectory($logs)
   & (Join-Path $PSScriptRoot 'Start-SteamVRBranding.ps1') -Manifest $manifest -Presenter $presenter -Log (Join-Path $logs 'steamvr-branding.log')
  }
 }
 & $launcher @launchArgs
 if($LASTEXITCODE -ne 0){throw 'The game did not exit normally. See TROUBLESHOOTING.txt and runtime\x86\logs.'}
 exit 0
} catch {
 Write-Host ('BF2142 VR: '+$_.Exception.Message) -ForegroundColor Red
 if($Action -eq 'Setup'){Write-Host 'If access was denied, close the game and run Setup.cmd as administrator.'}
 if($Action -ne 'Inspect' -and -not $NonInteractive){Read-Host 'Press Enter to close' | Out-Null}
 exit 1
}

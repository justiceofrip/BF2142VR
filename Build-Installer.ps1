[CmdletBinding()]
param([string]$Destination = '')
$ErrorActionPreference = 'Stop'
if (-not $Destination) { $Destination = Join-Path $PSScriptRoot 'build\installer-publish' }
dotnet run --project (Join-Path $PSScriptRoot 'src\installer-tests\BF2142InstallerTests.csproj') -c Release
if ($LASTEXITCODE -ne 0) { throw 'Installer tests failed' }
dotnet publish (Join-Path $PSScriptRoot 'src\installer\BF2142VRSetup.csproj') -c Release -r win-x64 --self-contained true -o $Destination
if ($LASTEXITCODE -ne 0) { throw 'Installer publish failed' }
Write-Output "Installer built: $Destination\BF2142VRSetup.exe"

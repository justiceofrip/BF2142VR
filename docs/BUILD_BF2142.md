# Building BF2142 VR

## Requirements

Windows 10/11 x64, Visual Studio 2022 C++ Build Tools (Desktop development with C++, Windows SDK, CMake tools), CMake 3.24+, and Git for repository work. The native build does not need game files or a headset. The pinned OpenXR/MinHook/d3d8to9 dependencies are included.

From the repository root, run each architecture in a fresh PowerShell process:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build-BF2142.ps1 -Architecture x86 -Jobs 6
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build-BF2142.ps1 -Architecture x64 -Jobs 6
```

The helper discovers Visual Studio, selects the proper compiler, uses Ninja if available (otherwise NMake), builds RelWithDebInfo, and runs the complete x86 CTest suite. Logs go to logs/; build output goes to build/. It enables BFVR_BUILD_BF2142_BOOTSTRAP for x86 and BFVR_PRESENTER_ONLY for x64. BF1942 reference targets remain in the x86 build. The x64 presenter executable retains its internal BFVRPresenter name.

Expected player outputs:

- build/bf2142-x86/bf2142/BF2142VRLauncher.exe and BF2142VRClient.dll
- build/bf2142-x64/BFVRPresenter.exe, assets/ and runtime/openxr/win64/openxr_loader.dll

Do not combine client/presenter binaries from different revisions. Protocol and input flags evolve together.

## Asset policy tests

Python 3.12 is used for player packaging. The mesh tests use standard Python and synthetic fixtures:

```powershell
Push-Location .\scripts\bf2142
py -3 -m unittest TestWeaponMeshes TestCompleteSurfaces
Pop-Location
```

GPU smoke executables are separate from CTest; see BF2142_PORT.md and the relevant native feature documents for arguments. Tests cannot prove headset comfort or online behavior.

## Build a player candidate

The portable packaging helper builds the standalone setup utility with Python 3.12.0, Pillow 12.3.0 and PyInstaller 6.22.3, then stages the already-built native outputs. It does not download any game content. Python must be installed for the developer; players receive the bundled utility and do not need Python.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build-PlayerPackage.ps1
```

Run both native builds first. Output is a new build/player-candidate/BF2142 VR Beta folder with a payload hash manifest. Existing staging folders are never overwritten. The helper emits a candidate for validation, not a public release. Record the exact package hash and finish the release checklist before publishing. Toolchain paths are removed from distributed PE debug filenames; native executable code is preserved.

## Run your build

Use the generated candidate's Setup.cmd on your own supported stock BF2142 v1.51 installation. Read START HERE.txt first. Setup creates the weapon/body/lobby assets locally and records rollback backups. Use the installed Play VR.cmd with a connected OpenXR runtime, or Desktop Preview.cmd for synthetic tracking. Never commit/share the installed BF2142VR directory.

For native integration work the launcher also accepts --game-dir, --presenter, --windowed and --inspect; see BF2142_PORT.md. Set BF2142VR_CONFIG to an absolute INI path for an explicit development configuration. Do not take historical profile addresses as proof for another game/version.


## Community helper and flat addon

Install .NET 10 SDK and the Visual Studio C++ workload for Native AOT. Players use the published EXE and do not need a .NET installation.

```powershell
dotnet run --project src/community-tests/BF2142CommunityTests.csproj -c Release
dotnet publish src/community/BF2142Community.csproj -c Release -r win-x64 -p:PublishAot=true -p:StripSymbols=true -o build/community-publish
```

Beta was built with .NET SDK 10.0.401 / Native AOT 10.0.12. Include .NET runtime and third-party notices when distributing the helper. Native voice links the pinned Opus source.

Flat payload needs `runtime/x86/BF2142VRLauncher.exe` and `BF2142VRClient.dll` plus licenses. It needs no presenter or generated assets. VR payload uses the player stage. `BF2142Community.exe pack` creates SHA-256 file/archive metadata; `sign` signs the reviewed server descriptor; `bundle-flat` embeds a single flat payload for offline setup. `bundle-join` embeds an HTTPS descriptor reference for online joining. See [host/integration](BF2142_COMMUNITY_HOSTING.md) and [flat contract](FLAT_ADDON.md).

Publish only reviewed payload files. Keep the publisher private key, server host state, native shared secrets, credentials, generated game assets and debug fixtures outside the repository/downloads. A public key in a helper must be chosen by its publisher; do not trust keys received from arbitrary servers.

The online join EXE is produced after its signed payload ZIPs, avoiding a self-referencing hash. In the VR user ZIP, this EXE and `Join Community Server.cmd` are distribution extras used from the extracted folder; the reversible asset installer copies only its `payload.json` file set.

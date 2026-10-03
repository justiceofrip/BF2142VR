# Installer and incremental updater

Download `BF2142VRSetup.exe` from the beta.4 release. Choose the installed
`BF2142.exe` once and click **Install / Update**. **Repair** verifies and restores
mod files and regenerates the local body/lobby assets. **Play VR** launches the
installed game. The app retains the game path and creates one Battlefield 2142 VR launcher
shortcut for play, updates and repairs. It checks the signed beta feed when opened; installation is
user initiated. A working installation remains launchable when the feed is offline.

Close BF2142 before updating. Changes are installed between sessions, not into
running processes. Healthy alpha/beta installations can be updated without a
manual uninstall. A corrupted rollback backup or changes by another mod stop
automatic replacement; originals are not guessed or overwritten.

The launcher uses Battlefield 2142 walker artwork selected by the project owner
and the original title-logo styling, with EA/DICE credited in the launcher.
Reclamation setup is linked visibly at https://battlefield2142.co/.

## Validation and reports

Check for updates verifies the signed release and the matching installed mod
file hashes. It distinguishes an available update from same-version corruption.
Play remains available if the feed is offline. Save error report exports setup
and launcher diagnostics, with common personal paths/credentials redacted.
Report on GitHub asks where to save it and opens an issue draft. The user must
review, attach and submit it. Nothing uploads automatically. Redaction is an
aid, not a guarantee that arbitrary third-party error text has no personal data.
Launcher copies use immutable content-based names outside the game runtime,
so updating does not overwrite a running launcher. The full ZIP includes it.

## What this fixes and what remains uncertain

The incoming setup log showed varying native process failures during weapon
repair, including access violations, breakpoint exceptions, fail-fast exits and
inconsistent Python exceptions. The earlier logging change exposed those
failures; it did not cure them. The report does not establish a single underlying
interpreter, driver or hardware defect. Do not claim one without reproduction.

Test.6 moves from bundled CPython 3.12.0 to
[CPython 3.13.15](https://www.python.org/downloads/release/python-31315/), removes
recursive generator traversal from the coverage query, and runs each weapon
repair in a fresh subprocess. Input and output must match the reviewed stock
hashes. Successful results are cached atomically; a failed weapon retries once
and then stops with its log. No mesh is skipped and the final archive must still
match the accepted full-archive hash. All generated weapon bytes remain identical.
Reporter confirmation of the original intermittent failure is still needed.

## Optional map water repair

Setup/update recognizes the known malformed Carbone Island water reflection
and rebuilds a valid cube from the owner's installed image. Other archive
entries remain unchanged; the original map archive is backed up and included
in rollback/uninstall. Missing maps and unfamiliar reflection images are skipped.
No map or water textures are downloaded with the mod. This does not add maps.

## Updates and rollback

- The executable contains a pinned ECDSA P-256 public key. The channel manifest
  must verify before any update file is accepted or executed.
- A signed manifest specifies version, monotonically increasing revision, file
  paths, lengths, SHA-256 hashes and GitHub release URLs. Download size, redirects,
  path traversal, duplicate paths and unexpected game/settings files are checked.
- Cached files and existing installed files are rehashed. Only missing, changed
  or damaged content downloads. Cancellation retains verified download progress.
- Setup stages the whole replacement beside the game. The first-install stock
  backups and `BF2142VR.ini` are copied forward, never replaced with patched
  files as the supposed originals. Settings asset paths are refreshed.
- The old runtime is retained in `.BF2142VR-previous-<transaction>` beside the
  game. An update journal restores it if a directory swap is interrupted.
  A per-game mutex excludes another install/update in the same folder.
- Uninstall still restores the verified original game mutations. Do not delete
  backup folders until you no longer need their rollback state.

The updater content cache and logs live under `%LOCALAPPDATA%/BF2142VR/Updater`
and `SetupLogs`. The separate `WeaponCache` contains generated owned-game models:
**never upload that cache**, game archives, `generated`, `backups` or credentials.

This is a preview-channel updater. It updates the standalone VR installation;
it does not reconfigure the community join helper/flat addon or provision a
server. It does not download Battlefield, map packs, Reclamation, Python or .NET
installers on a player's PC. Python/.NET components are bundled in the mod tools.
The updater itself can be replaced by downloading a newer installer EXE; the feed
can require a newer updater when its protocol changes.

## Build and publisher workflow

Build both native architectures and run all tests as in BUILD_BF2142.md.
Build-Installer.ps1 must run before Build-PlayerPackage.ps1. Stage a
player package with Build-PlayerPackage.ps1 using official Python 3.13.15 x64.
Its downloaded installer is checked against the python.org SHA-256 and PSF
signature before use. Review dependency licenses when changing any runtime pin.

Run `Build-Installer.ps1` with .NET SDK 10.0.401. The resulting self-contained
WinForms EXE includes .NET 10.0.12 and embedded license/third-party notices.
It is independent of the native game binaries. Test.6 ships the same gameplay
DLL/EXEs as test.5. Do not claim new headset fixes from this installer release.

Tests:

```powershell
$env:PYTHONPATH = "$PWD/scripts/bf2142;$PWD/scripts/bf2142/package"
python -m unittest TestWeaponMeshes TestCompleteSurfaces TestWeaponRepairWorker TestSetupUpdate TestWaterReflectionRepair
dotnet run --project src/installer-tests/BF2142InstallerTests.csproj -c Release
```

Finish exact-ZIP fresh install, upgrade from the previous published payload,
repeat repair, failure/rollback guards and uninstall restoration checks before
publishing. Validate the final standalone EXE through `--apply-offline PAYLOAD
--game GAME` in a private fixture. `--render-preview OUTPUT.png` renders the UI
offscreen without making shortcuts or downloading updates.

After validation, prepare the signed file feed (no upload occurs in this command):

```powershell
dotnet run --project src/installer-tools/BF2142InstallerTools.csproj -c Release -- prepare PAYLOAD PRIVATE_KEY REVISION RELEASE_TAG OUTPUT PREVIOUS_SIGNED_MANIFEST_OR_DASH
```

The private key stays outside the repository and distribution. `Publisher.pem`
contains only its public half. For subsequent releases, pass the last signed
manifest to reuse unchanged asset URLs. Upload the generated content-addressed
`files/file-<sha256>` assets and installer/ZIP/checks to that GitHub release first.
Publish it, then update only `test.json` on the `updates` branch. The fixed feed:
`https://raw.githubusercontent.com/justiceofrip/BF2142VR/updates/test.json`.
Never advance the feed to missing files, a draft release or an unverified package.

Verify the public delivery afterward with:

```powershell
dotnet run --project src/installer-tools/BF2142InstallerTools.csproj -c Release -- verify-online EMPTY_CACHE_FOLDER
```

Keep the public failure report anonymized. Raw setup logs and separate private
playtest notes are not part of source or release artifacts.

## Test.7 repair plans

`BuildRepairPlans.py` compiles only branch decisions (skip/append/subdivide and
keep/remove) from an owned, hash-identified stock archive. `WeaponRepairPlans.py`
contains compressed opcodes, not coordinates, indices, materials or textures.
`RepairDecisionPlan` bounds decompression and requires exact stream consumption.
The installer derives the same meshes locally and checks the original accepted
output hashes. Missing, truncated or altered plans fail closed; no silent slow
search fallback. Existing verified mesh caches remain valid. Full search stays
in the developer compiler and geometry tests. Gameplay is unchanged.

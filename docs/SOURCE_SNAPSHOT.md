# Source snapshot verification

BF2142 VR 0.1.0-alpha.1 candidate, v30 runtime, prepared 2026-09-23.

The clean source snapshot compiled independently with its documented Windows PowerShell helpers: x86 client/launcher/reference targets and x64 presenter passed. All 60 Win32 CTest cases and 15 Python mesh-policy tests passed. Build-PlayerPackage.ps1 built the bundled setup utility and staged the 118-file candidate manifest successfully. Native source files match the working v30 source tree; contributor/build documentation and packaging helpers were added for this snapshot.

The original player ZIP was separately tested for installation, repeat setup, rollback, refusal of later-mod changes/corrupt backups, asset reconstruction and archive integrity. See BF2142_ALPHA_READINESS.md. The clean-build packaging check does not replace the final original-ZIP headset acceptance. Builds include local paths in private debug/log output; those products are ignored and are not part of this repository.

The source snapshot contains source, tests, scripts, documentation, upstream runtime assets, pinned OpenXR loaders and third-party dependencies/licenses. It excludes local Git history, user configuration, private logs, generated game assets, game executables/archives, checkpoints and build output. The two pinned loader DLLs are intentional dependencies, not game binaries.

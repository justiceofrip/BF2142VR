# Battlefield 2142 VR

An early PC VR port of Battlefield 2142, based on [BFVR by JayBiggsGMG and the BFVR contributors](https://github.com/JayBiggsGMG/BFVR-Battlefield-1942-VR-Mod).

Current source: **0.1.0-alpha.1 candidate / v30**. The player installer has passed local setup and rollback checks. Final acceptance of the packaged build in a headset is pending. This repository is also a starting point for contributors interested in a Battlefield 2 port.

## Play

You need your own **Battlefield 2142 v1.51** installation and a working PC VR/OpenXR runtime. The initial playtest target is Quest controllers through Steam Link/SteamVR, with stock singleplayer/bot matches. Reclamation Hub/OpenSpy setup is separate; Remaster and multiplayer compatibility are unverified.

Use the player ZIP when available, extract it, and run `Setup.cmd`. Select the folder containing `BF2142.exe`, then use `Play VR.cmd` or the installer-created shortcut. Connect your headset before launching. A GitHub source download is for developers and contains no compiled player client.

- [Full installation instructions](scripts/bf2142/package/START%20HERE.txt)
- [Controls](scripts/bf2142/package/CONTROLS.txt)
- [Troubleshooting](scripts/bf2142/package/TROUBLESHOOTING.txt)
- [Release readiness and known limitations](docs/BF2142_ALPHA_READINESS.md)

## Implemented

- Stereo rendering and 6DoF head movement, recentering and recoil-free head rendering.
- Tracked weapons, local arm IK, two-hand support, body holsters and empty hands.
- Controller-touch finger poses, physical crouch/prone, snap/smooth turning and head/controller-relative locomotion.
- Physical ADS with supported optics, floating HUD, laser menus and a 3D walker lobby.
- Locally generated repairs for missing first-person weapon surfaces.
- Vehicle gun head aiming within stock articulation limits, seat controls and parachute comfort work.
- A reversible installer and loading-memory fix for the supported game profile.

These are alpha implementations. Remote players do **not** receive tracked arm IK. Hand-to-hand knife transfer, manual reloads and physical steering wheels are not implemented.

## Planned

Multiplayer compatibility and hit-registration checks, community Titan sessions, networked player IK, physical vehicle controls, optional manual reloads, and further hand/weapon polish. Custom cockpit modeling is a longer-term possibility. See the [roadmap](docs/BF2142_ROADMAP.md).

## Build and contribute

Start with [building BF2142 VR](docs/BUILD_BF2142.md). Windows C++ build dependencies are required; pinned third-party source and OpenXR runtime files are included.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build-BF2142.ps1 -Architecture x86
powershell -NoProfile -ExecutionPolicy Bypass -File .\Build-BF2142.ps1 -Architecture x64
```

For a BF2 port, read [BF2 porting guide](docs/BF2_PORTING.md). The presenter, tracking, input transport and interaction math provide substantial reusable groundwork. BF2's native renderer, object layouts and weapon/skeleton adapters still need their own verified profiles.

[Contributor guidance](CONTRIBUTING.md) / [developer handoff](docs/AI_DEVELOPER_HANDOFF.md) / [historical port notes](docs/BF2142_PORT.md).

## Credits and ownership

Original BFVR work: JayBiggsGMG and BFVR contributors. BF2142 adaptation maintained by justiceofrip. See [upstream provenance](UPSTREAM.md), [MIT license](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md).

No Battlefield executable, game archives, decoded meshes/textures or account data are included. Setup derives repaired models and lobby equipment from each player's own installation. Those generated assets remain game content and must not be committed or redistributed as mod source. This is a community project, unaffiliated with EA or DICE.

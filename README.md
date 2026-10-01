# Battlefield 2142 VR — beta.4-test.7 experimental branch

**[Download the installer / updater](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.4-test.7/BF2142VRSetup.exe)** — select BF2142.exe and click **Install / Update**. Later updates and repairs preserve your settings and original backups. [Full ZIP and checks](https://github.com/justiceofrip/BF2142VR/releases/tag/v0.2.0-beta.4-test.7) are also available.

Test.7 replaces expensive per-install geometry searches with verified repair instructions. Models still come from your own game and must match the same accepted hashes. Gameplay binaries are unchanged from test.5; this does not change resolution or FPS. Reporter confirmation of the remaining native installer crash is pending. Beta.3 remains the older gameplay fallback.

A PC VR port based on [BFVR by JayBiggsGMG and the BFVR contributors](https://github.com/JayBiggsGMG/BFVR-Battlefield-1942-VR-Mod). Play bot matches or join compatible multiplayer servers alongside desktop players.

**[VR ladder/input hotfix and downloads](https://github.com/justiceofrip/BF2142VR/releases/tag/v0.2.0-beta.3)** — [resolution / AA settings](docs/BF2142_RENDER_QUALITY.md).

| Download | Who needs it? |
| --- | --- |
| [BF2142-VR-0.2.0-beta.3.zip](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.3/BF2142-VR-0.2.0-beta.3.zip) | VR players: ladder/input fixes, quality update and reversible setup. Includes the remote-player IK receiver. |
| [BF2142-Flat-0.2.0-beta.1.zip](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.1/BF2142-Flat-0.2.0-beta.1.zip) | Monitor players: run the included EXE to join the community test server with tracked-player visuals. No SteamVR, headset or separate .NET installation. |

VR players do **not** install the flat addon too. Unmodified desktop clients can play on a compatible server, but need the addon to see VR gestures and use our proximity voice. The server also needs the VR server adapter and community bridge. This is a beta, with remaining animation and compatibility issues.

## Quick install

You need your own working **BF2142 v1.51** installation on Windows x64. Set up Reclamation/OpenSpy separately and confirm you can log in. VR has primarily been tested with Quest 3 controllers through Steam Link/SteamVR. Remaster and other weapon packs are not supported by this installer.

1. Close BF2142, open the test.7 installer EXE, select `BF2142.exe`, and click **Install / Update**. The full ZIP and `Setup.cmd` remain an offline alternative.
2. Connect your headset to SteamVR. Use **Battlefield 2142 VR Beta** / installed `Play VR.cmd` for singleplayer.
3. The separate [community helper](https://github.com/justiceofrip/BF2142VR/releases/tag/v0.2.0-beta.1) still selects its beta.1 runtime. Normal servers are selectable in-game from the updated Play VR launcher.

**Updating from alpha or an earlier beta:** use the test.7 installer’s **Install / Update** button. Healthy installations retain settings and original backups without a manual uninstall. If another mod changed game files or a backup is damaged, setup stops and preserves them. For a flat-addon update, use the new EXE instead of an older playtest EXE.

- [Full installation instructions](scripts/bf2142/package/START%20HERE.txt) / [controls](scripts/bf2142/package/CONTROLS.txt) / [troubleshooting](scripts/bf2142/package/TROUBLESHOOTING.txt)
- [Flat addon and Reclamation Hub integration](docs/FLAT_ADDON.md)
- [Beta checks and known limitations](docs/BF2142_BETA_READINESS.md)

## Features

- Stereo VR and 6DoF head movement, recentering and recoil-free head rendering.
- Tracked weapons/hands, local arm IK, two-hand support, body holsters and empty hands.
- Controller-touch finger poses, physical crouch/prone, snap/smooth turning, head/controller-relative movement.
- Physical ADS, floating HUD, laser menus, a 3D walker lobby and locally repaired weapon surfaces.
- Vehicle gun head aiming within native limits, seat controls and traversal comfort improvements.
- Multiplayer controller aiming; remote tracked head, arms, approximate finger curls and held/holstered weapon visibility.
- Smoothed observer IK, bounded torso follow and approximate cosmetic hand/arm self-collision.
- Fist-bump haptic implementation and separate equipment contact cues.
- Left-hand grip-and-throw medical/ammo crates with native cooldowns.
- Community join helper, signed/versioned downloads and encrypted pose/audio bridge.
- Proximity audio and physical shoulder interaction for native squad radio.

The latest observer arm/recoil and holster-haptic corrections have automated coverage but have not had another headset/observer session. The owner authorized this beta without that final test. Earlier builds were exercised in two-client IK and outside-network crossplay sessions. The owner confirmed proximity voice worked with the second `vrtester` client; separate-PC microphone routing and voice quality still need broader testing. Read the [known limits](docs/BF2142_BETA_READINESS.md).

## For contributors and BF2 ports

**Start with the separate [IK transfer guide](docs/ik/README.md)**: source map, engine boundaries, failure cases and regression workflow. See [changes since alpha](docs/BF2_POST_ALPHA_UPDATES.md) and the broader [BF2 porting guide](docs/BF2_PORTING.md). Reusable math does not make BF2142 memory offsets valid in BF2.

[Build instructions](docs/BUILD_BF2142.md) / [hosting](docs/BF2142_COMMUNITY_HOSTING.md) / [roadmap](docs/BF2142_ROADMAP.md) / [developer handoff](docs/AI_DEVELOPER_HANDOFF.md).

## Credits and ownership

Original BFVR work: JayBiggsGMG and BFVR contributors. BF2142 adaptation: justiceofrip. [Upstream provenance](UPSTREAM.md), [MIT license](LICENSE), [third-party notices](THIRD_PARTY_NOTICES.md).

No Battlefield executable, maps, game archives, decoded meshes/textures or account data are included. Setup derives its repaired models and lobby from each player's own installation. Generated assets remain game content and must not be redistributed as mod source. Community project, unaffiliated with EA or DICE.

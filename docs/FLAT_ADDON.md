# BF2142 Flat Viewer — desktop players

**[Download the launcher](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-flat-beta.2/BF2142FlatViewer.exe)** · **[ModDB ZIP](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-flat-beta.2/BF2142-Flat-Viewer-0.2.0-beta.2.zip)**

1. Have a working BF2142 v1.51 + [Reclamation](https://battlefield2142.co/) installation on Windows 10/11 x64.
2. Run `BF2142FlatViewer.exe`. It detects the game; use **Browse** to select your installed `BF2142.exe` if needed.
3. Click **Play** and use your normal login inside BF2142.

Use the **BF2142 Flat Viewer** desktop shortcut next time. Updates to the addon and launcher are automatic. The first launch downloads the signed addon. No manual DLL copying, headset, SteamVR, Python or separate .NET installation.

This addon shows VR players' head/arm movement, approximate fingers and held/holstered weapons. Desktop controls stay keyboard/mouse. **VR players already have this functionality.**

The launcher joins **our community crossplay server** directly and displays whether its bridge is reachable. VR gestures and proximity voice require the server adapter/bridge; the viewer does not enable these features on every Reclamation server. An unreachable server remains unavailable even with a healthy local installation.

The **proximity microphone** checkbox controls broadcasting to nearby players. New launcher preferences default to muted; an existing explicit setting is preserved. Native squad/team voice remains controlled by BF2142. Minimize the launcher during play; it keeps the helper connection alive. Closing its window during a match minimizes it.

**Repair** re-downloads the verified addon and retains the previous cache. **Copy error report** / **Save report** provide redacted launcher-only diagnostics. **Report on GitHub** opens a draft issue for you to review and submit. Nothing is uploaded automatically; accounts, game files and microphone audio are not collected.

Close the game and viewer to uninstall. Remove the shortcut and `%LOCALAPPDATA%/BF2142VR/FlatViewer`. Shared addon files live under `%LOCALAPPDATA%/BF2142VR/Community`; retain that folder if an older community helper still uses it. The viewer does not replace game archives.

The earlier beta.1 offline join EXE remains archived. It has manual updates; use this new viewer for automatic updates. The separate legacy VR community join helper still selects its older VR payload; use the main VR launcher for current VR fixes.

## Hub integration contract

The practical integration is a first-time opt-in component in Reclamation Hub. After that consent/download, Hub's Join action can start our helper with its selected trusted server descriptor and `--mode flat`. No VR setup is needed for desktop users.

```text
BF2142Community.exe join --server https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.1/server.json --mode flat --game "C:/Games/Battlefield 2142"
```

Use structured process arguments; substitute the user's game folder and a publisher-approved descriptor for the selected server. The helper locates the game when `--game` is omitted. It returns the launched game's exit code, or a nonzero value on validation/startup failure. Keep it alive for pose/audio transport throughout the match. Stop by closing BF2142 normally.

Hub should verify the initial helper against a reviewed release checksum or its own trusted update mechanism. Package signatures do not bootstrap trust in an arbitrary executable. The helper verifies ECDSA-P256 descriptors, SHA-256 archive/file hashes, bounded ZIP contents and a pinned server TLS certificate. Caches are versioned by archive hash and reverified on launch. Earlier files are retained; older descriptor revisions are rejected. An authorized rollback should be a new signed revision selecting the desired older payload.

Files live under `%LOCALAPPDATA%/BF2142VR/Community`; flat mode does not install VR assets or replace game archives. Optional `install` registers the current-user `bf2142vr://join?server=...&mode=flat` URI handler. `unregister` removes only this helper's owned registration. Registration is not needed for Hub's direct process launch.

Do not embed game passwords, private keys or server secrets. Normal OpenSpy authentication remains in BF2142. The embedded public publisher key is [Publisher.pem](../src/community/Publisher.pem); adding publishers requires a deliberate trust decision, not accepting a key from an untrusted descriptor.

A host needs the native BF2142 server adapter and community bridge; installing viewers alone does not create pose data. See [hosting](BF2142_COMMUNITY_HOSTING.md) for setup. The shipped helper trusts this project's publisher; independent hosts should coordinate signing or build with their own reviewed key. Reclamation integration is ready for review, **not already deployed in their Hub**.

## Historical beta.1 integration message

We've released BF2142 VR 0.2.0-beta.1, including a lightweight flat-player addon so desktop players can see VR players' tracked heads, arms, hand poses and equipped/holstered weapons. VR users already have the receiver; flat users don't need SteamVR or a headset.

The standalone join EXE is ready for review. For Hub integration, we can make this an optional one-time component, then have Join launch the helper with a signed server descriptor and `--mode flat`. It verifies/caches the addon and starts the game using the player's existing installation and normal OpenSpy login. The helper stays running for networking. Proximity voice worked in our two-client test; wider device/network testing remains.

Release: https://github.com/justiceofrip/BF2142VR/releases/tag/v0.2.0-beta.1
Integration/source: https://github.com/justiceofrip/BF2142VR/blob/main/docs/FLAT_ADDON.md
IK/BF2 notes: https://github.com/justiceofrip/BF2142VR/tree/main/docs/ik

Happy to coordinate the trusted update flow and your server-selection UI. Stock in-game joining cannot bootstrap the DLL; Hub integration would make subsequent joins seamless.

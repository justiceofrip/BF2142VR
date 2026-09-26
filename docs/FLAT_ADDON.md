# Flat addon and Reclamation Hub integration

## Players

[Download the flat beta](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.1/BF2142-Flat-0.2.0-beta.1.zip), extract it, close BF2142 and run `BF2142-Join-Flat-0.2.0-beta.1.exe`. Choose Play on desktop, select your own BF2142.exe if asked, and use your own Reclamation/OpenSpy login. Keep the helper open until the game closes.

This addon renders remote VR head/arm poses, approximate finger curls and held/holstered weapon state on a normal monitor. It includes proximity voice. It uses ordinary keyboard/mouse controls and needs no SteamVR, headset, Python or separate .NET installation. **VR players already have this receiver in the full VR mod.**

The bundled EXE targets the community test server. Its address can change and the test server is not guaranteed to be online permanently. The flat offline EXE includes its native payload and is updated manually by downloading a new release. An online helper is also available as [BF2142-Join-Community.exe](https://github.com/justiceofrip/BF2142VR/releases/download/v0.2.0-beta.1/BF2142-Join-Community.exe); it obtains a publisher-signed descriptor before selecting a cached/downloaded package.

The ordinary game server browser cannot install the addon automatically. A stock player can join a compatible server but sees stock animation without the addon. Joining through the helper starts the addon-enabled game. If the game is already open, close it before switching launchers.

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

## Copy-paste message to Reclamation

We've released BF2142 VR 0.2.0-beta.1, including a lightweight flat-player addon so desktop players can see VR players' tracked heads, arms, hand poses and equipped/holstered weapons. VR users already have the receiver; flat users don't need SteamVR or a headset.

The standalone join EXE is ready for review. For Hub integration, we can make this an optional one-time component, then have Join launch the helper with a signed server descriptor and `--mode flat`. It verifies/caches the addon and starts the game using the player's existing installation and normal OpenSpy login. The helper stays running for networking. Proximity voice worked in our two-client test; wider device/network testing remains.

Release: https://github.com/justiceofrip/BF2142VR/releases/tag/v0.2.0-beta.1
Integration/source: https://github.com/justiceofrip/BF2142VR/blob/main/docs/FLAT_ADDON.md
IK/BF2 notes: https://github.com/justiceofrip/BF2142VR/tree/main/docs/ik

Happy to coordinate the trusted update flow and your server-selection UI. Stock in-game joining cannot bootstrap the DLL; Hub integration would make subsequent joins seamless.

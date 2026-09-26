# BF2142 community join and hosting — beta

The beta helper is a standalone Windows x64 executable, built with .NET Native
AOT. Desktop players do not install .NET, SteamVR or a VR headset driver. A
server-specific executable embeds the publisher-signed server descriptor and
presents Play on desktop / Play in VR. It locates BF2142 once, verifies and caches
the required package, then starts the native launcher with a direct server join.
Normal Reclamation login and soldier selection remain the game's responsibility.

Unmodified BF2142 cannot download or inject this DLL from its own server browser.
The first run of the helper is unavoidable without cooperation from an already
installed launcher such as Reclamation Hub. Later `bf2142vr://join?server=HTTPS_URL`
links use the installed helper. A Hub integration can execute:

```
BF2142Community.exe join --server HTTPS_SIGNED_DESCRIPTOR --mode flat
BF2142Community.exe join --server HTTPS_SIGNED_DESCRIPTOR --mode vr
```

The URI handler accepts only `server` and `mode`, never filesystem paths or
commands. A server descriptor is publisher-signed ECDSA P-256 and includes
package archive/file SHA-256 hashes, version, size limits, game endpoint and
pinned TLS certificate. Only HTTPS downloads are accepted. Archives are staged
atomically and checked for extra/missing files, traversal, links, Windows device
names and expansion limits. Earlier package directories and the existing player
installation remain available. Cached binaries are rechecked before launch.
An older server descriptor cannot silently roll back an accepted revision.

## Network design

The existing native pose v4 and Opus voice v1 adapters remain loopback-only.
The public bridge uses TLS 1.2/1.3 control and separate-direction AES-256-GCM UDP
keys. UDP headers are authenticated; sequence windows reject replay, and stale
frames expire. The TLS clock midpoint avoids depending on matching PC clocks.

Before issuing a session, the bridge challenges a native player through the
game's ClientCommand route. The server plugin observes that command's native
issuer and a one-use 128-bit nonce. A local secret protects the proof receiver;
a roster generation and heartbeat revoke disconnected/reused player slots.
This relies on BF2142's legacy native player/command identity, not a new account
system or an independent audit of the original game's anti-cheat/network stack.
No account passwords are requested by the helper or included in pose packets.
The native host shared secret never leaves the server; client loopback tokens
are generated per launch. The game continues running through bridge reconnects.

## Windows server bundle

Supply your own BF2142 v1.51 Windows dedicated-server installation with the
[OpenSpy dedicated-server patch](https://docs.getbf2142.net/advanced/dedicated-server/install-server-patch).
The unpatched internet backend can crash the original server at startup. Game binaries,
maps and generated game assets are not included in the public addon.

1. Extract the community host bundle outside the game directory.
2. Run `scripts/bf2142/community/Prepare-Host.ps1 -ServerDir YOUR_SERVER
   -PublicAddress YOUR_PUBLIC_IPV4` on the host. It creates a new private state
   directory, TLS certificate, local tokens and a Titan maplist. Existing state
   is never overwritten. The native launcher performs its compatibility check.
3. Review ServerSettings.con and maplist.con. `network.ini` controls proximity
   range and whether nearby enemies can hear speech. Defaults: 20 metres, yes.
4. Run Enable-HostFirewall.ps1 as administrator on this host and allow the same
   ports in the provider firewall. Keep RDP limited to your own address.
5. Start-Host.ps1 starts the native game server plus bridge. Stop-Host.ps1 uses
   password-authenticated localhost RCON for a normal game shutdown.

| Public port | Purpose |
|---|---|
| UDP 17567 | Native game traffic |
| UDP 29900 | Native server query |
| TCP 17570 | TLS addon handshake/control |
| UDP 17571 | Encrypted pose and proximity audio |
| UDP 55124 | Native BF2142 squad/commander voice service |

Do not expose TCP47142 (RCON), UDP17568/17569 (local native adapters), UDP17572
(native connection proof), or UDP55125 (internal native voice control).

Prepare-Host backs up the native admin configuration, restricts its RCON bind to
localhost, and installs our separate Python module. It moves std_archive.md5 to
its backup folder so both original flat archives and repaired VR weapons can
join; map checksum files remain active. This allows modified common archives,
not a cryptographic whitelist of one approved game asset build. Our plugin uses
the game's overlay admin configuration path: its restricted `os` module does
not expose `environ`.

Keep `host.json`, `network.ini`, `proof.key`, `host.pfx`, native admin/default.cfg,
and admin/bfvr_community.key private. Never include these in player downloads.
Only `server.unsigned.json` is turned into a public descriptor after attaching
approved flat/VR package metadata and signing with the off-server publisher key.

## One-month low-cost test

The selected target is Lightsail Windows Server 2022, 2 GB RAM, dual-stack IPv4,
$22 monthly base price (verified 2026-09-24). This is an initial small-match
sizing choice, not a measured capacity claim for 64 players. Windows consumes
most of the RAM; the local eight-bot dedicated game process measured about 60 MB.
No cloud GPU is needed because player PCs render their own views.

The budget is approximately $25 for one test month, not ongoing hosting. In an
authenticated AWS CloudShell, run Schedule-TestExpiry.py with the instance's
region after creating bf2142-vr-test. It checks the selected Windows plan is at
most $25/month, creates IAM permissions limited to deleting that exact instance
ARN, and verifies two deletion schedules at day 29 and 12 hours later. It does
not create or resize a paid server. Save wanted configuration locally before
expiry: deletion removes the cloud disk. Stopping an instance alone does not end
Lightsail billing. Taxes and usage beyond the included allowance are additional;
an AWS budget notification is useful but is not a hard spending cap.

Primary references:
- https://docs.aws.amazon.com/lightsail/latest/userguide/amazon-lightsail-bundles.html
- https://docs.aws.amazon.com/en_en/lightsail/latest/userguide/amazon-lightsail-frequently-asked-questions-faq-billing-and-account-management.html
- https://docs.aws.amazon.com/scheduler/latest/UserGuide/managing-targets-universal.html
- https://www.battlefield2142.co/faq/

## Verification status

The first native x86/x64 builds pass all 71 native CTests. The managed fixtures
passed 279 checks, including an actual local TLS-authenticated encrypted UDP
pose/voice exchange, ownership rejection, replay/tamper/expiry, signed downloads,
cache tampering, ZIP isolation and HTTPS downgrade rejection. The first Native
AOT helper is approximately 6.5 MB. Native ClientCommand admission and an actual flat game connection to the
public cloud have also passed. Outside-network flat/VR gameplay and remote IK have been exercised. The owner confirmed proximity voice with the second vrtester client. Latest arm/haptic changes await visual confirmation; broader separate-PC audio routing/quality needs testing.

## Map controls

On the host, run `scripts/bf2142/community/Manage-Maps.ps1 -Action List` to see
its current rotation. Use `-Action Change -Map Minsk` to select a listed
map, or `-Action Next` to advance. A change ends the round for connected players.
The tool reads private host credentials locally; no public RCON port is needed.
Save persistent rotation entries in the configured maplist.con. Each standard
Titan layout uses `gpm_ti 48`; this layout identifier is separate from maxPlayers.
Changes to sv.maxPlayers require restarting the game server.

Installed standard Titan maps do not supply bot navigation. Increasing slots or
the bot-count setting does not create working Titan bots. AI-capable map content
and its client compatibility need separate setup and validation. The owned cloud is configured for 64 slots and has run 62 stock co-op bots.
This does not establish 64-human capacity on the 2 GB hosting plan.

For the first bot match, stock Suez, Belgrade, Cerbere, Berlin and Verdun provide
co-op AI using `gpm_coop 16`; the layout size is separate from server slots. Other
installed modes, including Wake Island, are human-only. All 20 installed maps
are in the owner's rotation with voting enabled. Use the co-op entries for bots.

## Offline flat playtest delivery

`bundle-flat --helper PATH --archive FLAT_ZIP --manifest SIGNED_JSON --out EXE`
appends a verified single flat package and signed descriptor to the NativeAOT
helper. It verifies hashes again before extracting, and reuses the normal cache
and launcher. No hosted download or .NET install is required. Offline updates
are manual. This is intended for the first private flat tester; it does not
replace the online signed-descriptor/update path. Do not include private host
credentials or original game files. The flat dialog explicitly discloses that
nearby players can hear the microphone during gameplay.

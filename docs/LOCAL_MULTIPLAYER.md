# Local multiplayer development

Current public beta: [IK guide](ik/README.md), [flat addon](FLAT_ADDON.md) and [community hosting](BF2142_COMMUNITY_HOSTING.md). Native transports below stay loopback-only; the authenticated community bridge provides internet transport. Older private-candidate notes are historical. Shared presenter IPC is now 26.

A single Windows PC can run the dedicated BF2142 v1.51 server plus one desktop
VR simulator client. The dedicated server supplies authoritative bots and game
state, so a second person is not needed for initial connection and firing work.
This does not establish internet compatibility, remote VR poses or Titan play.

## Receive-only observer and remote empty hands (v33f, private)

The launcher now supports --network-observer: native flat rendering/input, no
OpenXR session, synthetic controllers or local VR controls. The matching
protocol-v4 server accepts a separate authenticated Subscribe lease and relays
other players' poses to it, including while the observer is unspawned. Leases
expire after one second, have sequence/session/port checks, and never provide
tracked fire, movement, snap or throw authority. This is still loopback-only.

For two clients on one PC, add --observer-profile "ABSOLUTE ISOLATED DOCUMENTS"
and --join-local 17567 to the observer. Before native main resumes, this child's
SHGetFolderPathW Documents lookup is redirected and an exclusive profile lock is
held for its lifetime. Primary Documents/profile paths are rejected. Only this
explicit observer mode allows the additional instance (+multi 1); ordinary
launches keep their existing duplicate-process guard. Use a separate account.
Launch the normal primary game first, then the isolated observer. Close the
observer before restarting a normal primary launch.

The first flat observer hit RendDX9_ori+0x127c during loading. That path lacked
the existing native query guard used by stereo. Observer startup now installs
that same signature-checked guard independently; the rerun loaded successfully
and logged the missing-query fallback without crashing at that point.

Fresh remote empty-hand poses now suppress only the held weapon's own bundled
mesh submission and shadow callbacks. The verified geometry owner is +0x290;
weapon +0x44 points back to it. Renderer vtable +0x1d5400 slots +0x18/+0x20 and
function signatures guard the draw hooks (+0xc5030/+0xc67e0). The decision checks
current inventory, native player/weak/soldier ownership, mount/death state, pose
age, held flags and weapon name. Explicit AI mirror diagnostics permit a different
bot weapon; normal relays require matching human weapon identity. No shared mesh,
material, skeleton, inventory or gameplay flags are modified. Missing tracking,
weapon switches and stale/unknown owners retain native drawing on the next call.
This candidate still needs visual acceptance with actual VR input.

68 Win32 CTests pass, including separate UDP observer relay, no observer action
publication, native mesh/shadow call forwarding and owner/dropout transitions,
and real process-local Documents redirection/exclusive locking. x64 builds.
The live lab ran two BF2142 processes simultaneously with distinct accounts;
both connected to the dedicated server. The second profile opened with empty
login fields and stored files under its isolated Documents root. Its query and
remote-weapon draw profiles installed successfully. Two connected flat clients
are not proof of headset-to-observer IK/empty-hand appearance or internet play.

The previous bot video proves transmitted arm motion, but visual IK quality is
inconclusive: the bot retained its gun/combat AI, and the temporary position
fixture placed it on a railing. Do not repeat that fixture as a visual acceptance
test. Remote fingers/head retargeting, haptic fist bumps and voice remain pending.

## Current developer launcher

`BF2142VRLauncher.exe --game-dir "YOUR GAME COPY" --windowed --desktop-vr --join-local 17567`

`--join-local` accepts only a numeric port from 1 to 65535 and targets
127.0.0.1. It supplies the game's native +joinServer and +port arguments.
The game can still require login and a manual Connect to IP operation.
The ordinary single-client guard remains enabled. --inspect never starts a game.
Use the existing desktop simulation keys; SteamVR is unnecessary.

Use a separate game copy to keep development assets away from the player install.
That copy still uses the Windows user's normal BF2142 login/profile directory.
The native +overlayPath option did not isolate that directory in our first
Windows-client launch; do not rely on it for separate accounts or concurrent
clients. Two simultaneous clients remain future work.

## Private lab setup and evidence

The local workspace contains a separately extracted v1.51 unranked server,
Suez_Canal/gpm_coop/16, eight bots, internet listing disabled, loopback game
socket on UDP17567, and password-authenticated loopback RCON on TCP47142.
The engine's query socket uses UDP29900. The LAN browser can discover the
machine's network address even though its game socket is loopback-only; use
Multiplay > Online > Advanced > Connect to IP, then enter 127.0.0.1
and port 17567 for this lab. See the [Reclamation FAQ](https://battlefield2142.co/faq/).

Authenticated developer telemetry reads the stock PlayerManager counters.
BulletsFired and bulletsGivingDamage are sequences of (weapon, count), not
integers. The helper preserves those sequences and sums them for display.
Counters may reset during the game's normal statistics processing.

Verified: dedicated server boots, loads its map, registers eight bots, answers
RCON and appears in the local browser. Desktop VR client boots with the current
native hooks. The LAN-browser join failed. Direct loopback connection registered a human,
then the stock content check rejected the VR assets. The private server now
backs up/omits std_archive.md5 to allow modified common archives; map checksum
files remain active. A subsequent direct join reached the match: the server recorded an alive
remote human, respawn/death, vehicle entry/exit, and fired rounds. Damage
alignment was still unverified at that stage; see the later combined session below.
Native x86/x64 builds passed. Build tests alone establish neither hit consistency
nor visible remote IK, simultaneous clients or external reachability.

Keep server binaries, game copies, generated assets, logs, account data and
RCON credentials out of the public repository and player release.

The local modded-content setup follows the server author's
[BF2142 server modding instructions](https://www.moddb.com/mods/crazy-ballistics-mod/tutorials/server-side-modding-tutorial),
limited here to the common-archive checksum manifest. It disables that stock-only
content gate; it is not a whitelist of this specific VR build.

## Experimental combined aim and arm-IK checkpoint

The development source now includes an optional native dedicated-server DLL and
launcher, plus a separate third-person arm solver. These are private lab tools,
not part of the published alpha package. The server adapter has run with actual
headset/controller poses. Visual hit alignment and two-real-client pose visibility
remain pending.

Build the normal x86 targets, which now also produce BF2142VRServer.dll and
BF2142VRServerLauncher.exe. Set BF2142VR_NETWORK in both launch environments to
an absolute private INI path, then launch the dedicated server:

```text
BF2142VRServerLauncher.exe "SERVER DIRECTORY" "SERVER SETTINGS" "MAP LIST"
```

The optional final --check argument initializes the extension in a newly created,
suspended server process, then closes that process without starting a match. It
checks the actual server image/profile and loader; it does not establish gameplay.

Private configuration example (generate a unique, nonzero 32-hex-digit secret;
never commit or distribute your actual token):

```ini
[Network]
Enabled=1
Port=17568
Secret=YOUR_32_HEX_DIGIT_PRIVATE_TOKEN
MirrorToBot=0
```

Disabled/missing/invalid configuration leaves the ordinary client behavior intact.
Both transport sockets bind only 127.0.0.1. This checkpoint is deliberately a
one-PC lab and cannot carry tracked poses to a second PC yet. A shared local token
is a development guard, not a replacement for authenticated multiplayer accounts.

The client sends canonical head/palm/held-weapon poses relative to its soldier,
using bounded, versioned packets without pointers. The server checks the live
human slot, active weapon, body position, sequence/session, ownership and pose age.
Its signature-checked pre-velocity launch hook applies controller aim to native
infantry projectiles; native speed, spread and inherited movement remain. Missing
or stale tracking falls back to native fire. Vehicles keep their existing native
input path. Networked melee gesture reconciliation is not established.

Other opted-in clients receive poses for separate 80-bone third-person arm IK.
The accepted 70-bone local first-person hand solver remains the source of the
canonical palm positions. Remote animation LODs collapse some wrists/fingers, so
the renderer reconstructs those joints from the same detailed local 80-bone rig
when needed. Legs, torso and native locomotion remain game-driven. Head poses and
finger curls are transmitted but are not yet retargeted onto remote head/fingers.

Explicit MirrorToBot=1 relays the local human's poses back under a nearby bot's
slot. This lets one person inspect the transmitted arm solver in one headset
session. It is a diagnostic substitute for an observer, not evidence of two real
VR players. The bot keeps its native movement and combat AI, which can differ from
the mirrored hands. A bot corpse/vehicle is not a reliable viewing target.

Validation so far: 63 CTests including actual loopback round trips, malformed and
stale packet rejection, native roster/owner fixtures, pre-velocity launch/parent
mapping, and isolated remote-arm math. Five private captured game rigs also pass,
including remote LODs. The actual dedicated executable passes suspended extension
initialization. These checks do not establish visible in-game IK or hit alignment.
Keep all tokens, captured rigs, native assets, private server config and logs out
of the repository. The combined v31 checkpoint is separate from normal shortcuts
and the published player ZIP.

Dedicated runtime finding: v31's animation-finalize receive pump never ran on the
headless server (confirmed by its live counter). The v31b server instead pumps
from signature-checked PlayerManager lookup/list queries on its owning game
thread, plus a pre-fire receive step. It keeps native query results unchanged.
The server can restart independently while the existing headset client stays
running; the local pose token must stay unchanged across that restart.

Live combined headset session: the client connected/spawned on the private server,
published poses, and the server logged accepted human poses and tracked launch
frames while firing. This establishes the authoritative adapter executing, not
visual hit alignment. isAI is player+0xd4 (vtable slot 26), verified against all
eight bots and the human; +0x3d1 was a different flag. v31c includes that correction
and preserves the same network token. During the live session only the exact
source-equivalent operand in our server DLL was corrected with its game thread
briefly suspended; no native game flag or asset was edited. Remote arm visibility
still needs an active, focused headset view; focus loss deliberately stops sends.


The remote client now hooks the verified third-person finalizer at EXE+0x1efc00;
its native arguments/return ordering are retained. The existing 1P callback at
EXE+0x1ee980 is insufficient for remote actors. The third-person callback also
pumps reception when a local 1P callback is absent. The new native-layout fixture
covers callback ordering, actual palette writes, local/other-thread/stale guards,
invalid topology, LOD reconstruction and observer roster loss. A target beyond
one arm's reach leaves that arm native without dropping the other arm's pose.

Runtime evidence is separate from tests: the headset session generated accepted
poses and over 120 native tracked-launch events. One later server snapshot showed
98 shots, six damage hits and one kill. These counters can reset and do not prove
bullet/sight alignment or identify teamkills. The desktop simulator then joined
Suez and Fall of Berlin, sent controller fixtures through the real server relay,
and reached successful 80-bone arm writes. Some poses fell back to native animation;
a running/crouching mirror bot can differ from the local player's stance. These
logs are not proof of stable visible IK or two actual VR clients.

No headset is required for --desktop-vr adapter checks. The current simulator's
mouse-driven menu ray assumes an unrotated menu anchor: after changing simulated
head yaw, use End then Home before interacting with menus. This is a development
workaround, not a change to headset menu behavior. Controller and visual acceptance
still require a real headset later. The owner has deferred the Instant Action
feature comparison while this network work is completed.

A captured remote rig exposed small scale/shear errors (about 0.7%) in native
animation blends. Applying a transpose as the inverse amplified that error and
could reject otherwise reachable arms. InverseAnimatedBone keeps the existing
near-rigid acceptance threshold but inverts the accepted 3x3 basis exactly. It is
used only for remote native bone/reference transforms; network pose validation
and accepted first-person math are unchanged. The previously failing captured rig
and a deterministic blended-rig regression now pass.


After the blend correction, the saved v31f desktop client sent simulated controller
poses through the actual dedicated-server relay to an on-screen bot. A bounded
private fixture temporarily paused AI and positioned one bot nearby, then restored
normal behavior. Raising the simulated right controller visibly raised the bot's
arm and weapon; the left hand remained tracked. In 160 unsynchronized memory
samples of the neutral pose, left/right wrists were within 2 cm of their targets
in 160/157 samples; a raised-pose run gave 160/144. These reads can catch native
animation between updates and do not distinguish every transient fallback. They
establish live pose-driven arm rendering, not perfect stability or headset feel.
No second actual VR player was involved. The fixture, native captures and images
stay private and are not required by the mod. The exact v31f headset check, visual
hit alignment, external transport and two-real-client checks remain pending.

## Physical ADS and local equipment regression fixes (2026-09-24)

The direct client zoom setter works in instant action but is corrected away by
an authoritative dedicated server. The experimental network client now sends
bounded native alternate-fire input for automatic ADS. The 180 ms alignment /
300 ms lowering hysteresis is unchanged. Native acknowledgement is awaited;
server/reload/sprint cancellation blocks re-entry until the player lowers the
weapon. Queued entry is canceled on tracking loss. Menu input and another or
stale weapon never consume an ADS press. The offline zoom path remains intact.

The copied private lab INI had corrupted UTF-16 newlines and stale generated
asset paths. Correctly decoding/re-encoding it restored all 56 local equipment
models. Holster interaction and rendering are client-side features, independent
of replicated remote arm poses. The preparation script now respects the BOM.

Desktop tests in the actual dedicated match kept physical ADS enabled for the
full 12-second sample, released on lowering, re-acquired, fired while scoped,
and recovered from simulated tracking loss. A downward capture shows the chest
knife and belt equipment. All 63 CTests and the x64 presenter build pass. These
are desktop observations; headset acceptance and two real clients remain open.
Voice chat / Quest microphone routing / VR push-to-talk are deferred; see the
[roadmap](BF2142_ROADMAP.md).

## Private movement heading correction (v33b, protocol v3)

The client and server use the same movement-only camera-basis adjustment for
walking and sprint. Native input, speed and camera/body storage stay unchanged.
MovementValid and movementYawDegrees are appended to the private pose packet;
version 3 is 616 bytes and rejects version 2 or malformed offsets. The server
checks human ownership, foot state, current weapon, thread and 150 ms freshness.
Unmodified flat clients and bots have no accepted VR movement packet and retain
ordinary movement. Roll back both DLLs when returning to a version 2 checkpoint.
This has no effect on the released singleplayer alpha or public server policy.

## Physical squad radio candidate (v33i, private)

The private server initially had sv.voipEnabled 0. Its backed-up config now enables
built-in VoIP (sv.voipServerRemote 0), quality 3, using stock ports 55123-55125.
Changing the live setting alone did not open the service ports; restart the lab
server before the audio check. Both native client profiles already enable VoIP
and push-to-talk. No microphone device or open-mic preference was changed.

In VR, reach to the upper-left shoulder with your left hand and squeeze the grip.
The hand seats on the radio, curls around the casing and presses its talk switch;
a left-controller haptic click accompanies native V being held. Release to stop.
The right-hand gun remains usable. A held support crate blocks radio pickup;
tracking/focus/menu loss or recenter cancels talk. No stick-click voice chord.
Flat players retain stock V. Both clients must join the same squad for this test.
Native voice settings retain volume/mute authority. Native commander/leader B
is untouched; there is no added commander gesture yet.

Shared x86/presenter IPC is v24; replace both together. Pose transport remains v4.
69 CTests, x64 build, D3D9 depth/state stereo fixture and captured hand solves pass.
Headset interaction/appearance and actual microphone delivery are not accepted.
Voice-activated proximity with no held binding is requested but not implemented.

## v33g: standing recenter and tracked remote head (private)

The next private checkpoint adds explicit standing-height recenter and remote
HMD yaw/pitch/roll. Head rotation keeps the native neck attachment and is independent
of arm reach. Protocol v4 and the receive-only flat observer remain unchanged.
Automatic focus/spawn recenters do not silently redefine standing height.

For the combined visual check, stand normally and click the right stick once,
then crouch and stand. On the observer, check a slow nod, left/right look and head
tilt, followed by an empty-hand wave and weapon redraw. Voice work follows this
candidate; launcher-managed addon downloads are deferred. Unmodified flat clients
do not contain the tracked avatar receiver. Internet transport is still pending.

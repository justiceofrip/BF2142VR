# Local multiplayer development

A single Windows PC can run the dedicated BF2142 v1.51 server plus one desktop
VR simulator client. The dedicated server supplies authoritative bots and game
state, so a second person is not needed for initial connection and firing work.
This does not establish internet compatibility, remote VR poses or Titan play.

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

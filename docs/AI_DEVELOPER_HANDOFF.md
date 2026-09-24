# BFVR AI and Developer Handoff

This is the primary continuation guide for human developers and AI coding
agents taking over BFVR. It explains what the major pieces do, which behaviors
must be preserved, and how to verify changes. It is intentionally more current
and task-oriented than the chronological `devREADME.md`.

## Multiplayer turn/crate/render work: v33

The accepted v32a private checkpoint remains the rollback. Protocol v2 appends
bounded snap and support-throw events to soldier-local pose packets; it requires
a matched client/server pair. A pose sequence is not an action event ID.
Retransmissions must never apply a turn/throw twice. Snap events feed only the
dedicated horizontal input getter's verified return site (0x12d49e); no soldier
body matrices or separate VR reference-yaw offsets are written. On the native
desktop test the client and server both settled to the same 30-degree turn.

SupportCrates contains only the accepted SP utility policy. Do not import
SandboxActions, manual-reload changes or blue-rifle-sight artwork into this
multiplayer change. Read native replenishing-ammo energy before showing/grabbing
a stored crate. Let the normal weapon-selection/fire path own its cooldown and
spawn; the guarded crate launch/fire adapters supply left-palm pose and velocity.
A held crate has its own item transform and leaves both tracked wrists independent.
Protocol events are disabled unless the existing private network client is active.

RenderStereo must not call the flat native camera just because its XR request or
consumer fence is pending. Preserve the native outer Present behavior and
outside-render menu capture. Preserve native rendering fallback when VR actually
fails, and preserve ordinary loading/menu rendering.
The real GPU fixture tests stalled consumption, scope/reflex/MSAA, extra Presents,
opaque native time-argument preservation and paused-menu resume. Never clamp or
accumulate an unknown native render argument while skipping a request. Keep the
accepted renderer timing on every completed pair.

Verification so far: x86 build/64 CTests, x64 presenter build, 13 real D3D9 GPU
cases, a 30-degree dedicated/client heading match, and a native server-created
left-hand medkit throw at the measured controller release velocity. Headset-visible
completion and two real clients still require testing. A desktop-only viewmodel
visibility issue reproduced with both v33 and the saved v32a DLL in the same lab;
do not attribute it to the utility port or claim headset visual validation.
The scene-flicker fix addresses a proven flat-render fallback at the XR consumer
fence; whether it completely resolves the owner's headset report is still pending.

## Multiplayer regression checkpoint: v32a

The owner reported physical ADS dropping out after a fraction of a second and
invisible local chest/belt equipment. A read-only desktop native-state capture
reproduced automatic zoom 0 -> 1 -> 0 after about 125 ms; an ordinary mouse
alternate-fire press remained zoomed. NetworkClientActive selects AutoAdsInput:
use the existing DirectInput overlay to send a 120 ms native toggle and allow
750 ms for native acknowledgement. Never call the local-only zoom setter in
this mode. Keep the accepted offline adapter, optics/eye rendering, first-person
hands and server firearm/IK code unchanged. Menus/mounts/lost focus suppress
input; tracking loss cancels queued activation; only the still-local, validated
weapon may receive an owned exit on safe gameplay return. Unknown weapons and
manual zoom retain native behavior. This assumes the lab's stock RMB alt-fire
binding, just like the existing controller input adapter.

Private lab preparation had incorrectly rewritten UTF-16 bytes as text, corrupting
newlines and leaving copied asset paths behind. Its INI and preparation script
are repaired; runtime loads 56 equipment models. These are local render props;
network equipment replication is unnecessary. Native multiplayer desktop capture
shows chest knife and belt props. Do not put those game-art captures in source.

Verification: full x86 build / 63 CTests (including delayed input acknowledgement,
manual ownership, native cancellation, tracking loss, stale weapon and timeout
coverage), x64 presenter build, 12-second native ADS sample, firing, lowering,
reacquisition and simulated tracking dropout in a real dedicated match. Exact
headset feel and two real VR clients remain pending. Active private lab shortcut
selects v32a; normal v30/public alpha remains unchanged. Voice chat is explicitly
deferred to the roadmap. The local server still has voice disabled.

## Local multiplayer development

See LOCAL_MULTIPLAYER.md. Developer launcher adds --join-local PORT, targeting
127.0.0.1 through native CLI arguments. It preserves the normal single-client
guard. A private dedicated v1.51 co-op lab boots with eight bots and authenticated
RCON telemetry. Direct loopback join reaches the server, but its stock content
check rejected the VR archive. The private std_archive.md5 is now backed up and
omitted; map checks remain active. Reconnection reached gameplay: the server
recorded an alive remote human, death/respawn, vehicles and fired rounds.
Experimental implementation now lives in src/bf2142/multiplayer: a loopback-only
pose channel, native dedicated server launch/fire hooks, and a separate 80-bone
remote arm solver. See LOCAL_MULTIPLAYER.md for explicit configuration, native
profiles, LOD handling, test-mirror limits and verification. The accepted 1P hands
only publish canonical poses; no first-person math was replaced. 63 CTests, x64
build, five captured native rigs and actual-server initialization pass. The real
headset session connected/spawned and exercised the server's tracked launch hook;
server telemetry recorded 98 fired rounds, six damage hits and one kill in a
snapshot. This does not establish visual hit alignment. A subsequent desktop
simulation exercised actual server relay and native third-person arm writes.
Visible raised-arm rendering through the real relay is now confirmed in the
desktop bot-mirror fixture. Exact-build headset feel, consistent visual alignment
and two real VR clients remain unverified. The rejected
profile-isolation experiment is not in this source: native +overlayPath did not
isolate Windows client profiles. Released alpha ZIP and accepted shortcuts stay
unchanged. The lab is on local branch dev/local-multiplayer, not a new release.

## Player package: 0.1.0-alpha.1 public early playtest

The player ZIP is public at https://github.com/justiceofrip/BF2142VR/releases/tag/v0.1.0-alpha.1.
The owner explicitly approved publishing the unchanged candidate as an early
playtest, waiving the final packaged-headset checklist gate for this release.
The check is still pending, not passed. Source and player downloads are now
available under justiceofrip/BF2142VR. All six release assets match local SHA-256
hashes; no game archives, installed data or backups were uploaded. The packaged launch reached native hooks and
assets, but OpenXR could not start while Steam Link was disconnected. SteamVR
was started and reports Awaiting Wireless Connection (215). Await an actual
headset connection before relaunching; do not treat elapsed time as acceptance.
Keep the normal v30 developer shortcut and all immutable checkpoints intact.
This clean source snapshot includes portable Build-BF2142.ps1 and
Build-PlayerPackage.ps1; see BUILD_BF2142.md, BF2_PORTING.md and SOURCE_SNAPSHOT.md.

scripts/bf2142/package contains SetupAssets.py, Player.ps1, StagePlayer.py and
player instructions. Setup uses a bundled Python/Pillow executable, not system
Python or downloaded game files. Optional decoder callbacks in the asset exports
preserve the original ffmpeg path. The newly generated weapon archive matches
accepted v25 exactly; lobby geometry/rendering is checked, texture resize now
uses Pillow in the player setup. Private generated packs must not enter source.

Setup stages under a unique game-local directory, validates the payload and
stock v1.51 profile, prepares backups then atomically replaces the weapon
archive and only the LAA header bit if absent. Installation metadata records
original/installed hashes. Uninstall validates every change/backup before
restoring any file and stops on later mod changes. No Hub redirect, account,
map or player profile changes are made. A separate Alpha shortcut is optional.
The installed folder retains backup/settings after uninstall by design.

Validation: 15 mesh-policy tests, real asset regeneration, new lobby GPU fixture,
fresh stock install in a path with spaces, LAA enable/restore, native --inspect,
repeat setup, later-mod and bad-backup protection, byte-exact uninstall and
repeat uninstall. Actual ZIP extraction and accepted-repair upgrade install pass.
Read BF2142_ALPHA_READINESS.md for remaining acceptance, and BF2142_SHARING.md
for distribution boundaries. The candidate ZIP excludes all game art and local
PDB paths; runtime executable sections retain v30 code. The ZIP is not the
installed BF2142VR folder. Do not distribute generated/backups/install.json.

## Current BF2142 checkpoint: v30

The owner reports v29 loading is substantially more stable. Remaining reports
were stock arm animation while gliding, vehicle weapons not following head aim,
and no discoverable seat/pod bindings. v30 handles those together.

NativeVehicle reads the exact alive local soldier and controlled stock named
PlayerControlObject; no remote object is modified. Verify the PCO constructor,
instance/template vtables and getter signatures. Parent chains are bounded and
cycle checked. Walker gunner ancestry includes the rotational-bundle override
at vtable +0x5626d8 / getter +0x1636f0; preserve that signature-gated exception.
Root +0xa0 is the chassis transform, not the rotating turret transform. Native
camera +0x40 and all module/player/seat identities remain checked.

VehicleView holds entry camera position/yaw relative to chassis. The stereo
path composes HMD motion once. VehicleControls feeds bounded native mouse
correction from target gaze minus native aim, without integrating at traverse
limits. Native vehicle axes stay vehicle-relative; infantry crouch must not
activate vehicle free-look. Aircraft pilots keep normal flight axes, buggy
steering stays native, Nekomata main gun gets head pitch only. Other known
movable-gun seats follow yaw/pitch. Unknown/modded templates fail closed.
Left trigger + X selects driver/F1; + Y advances F2-F8 per button edge. Native
seat availability wins. APC pod launch is PIAltFire/right grip, Titan launcher
PIFire/right trigger (also PIAltFire). See BF2142_VEHICLES.md.

Only a proven local Parachute traversal allows mounted first-person hand writes.
Keep the existing 70-bone topology/owner checks and leave camera/root bones
untouched. Use traversal comfort in skeleton space, show empty tracked hands,
exclude parachute from firearm pose publishing, and freeze gun grip settlement
until back on foot. Ladder and unrelated mounts remain excluded from hand IK.
Stereo reset clears the vehicle anchor. Desktop brackets simulate head yaw;
Left Shift + X/Y simulate controller seat chords without leaking physical keys.

Validation: Win32/x64 built, 60/60 CTest, 13/13 render fixtures; read-only live
profile and actual parent-chain inspection. No v30 live interaction or headset
acceptance is claimed. The owner is using the desktop; do not interrupt them
for repeated small tests. The next normal launch selects v30, v29 is rollback.
Preserve v29's LegacyShaderMemory byte-for-byte and immutable older checkpoints.

## Preserved BF2142 v29 loading stability

The owner generally accepted v28 and the direct OpenVR OBS capture quality, but
reported recurring loading crashes independently of recording. Five recent
local crash dumps include the retail renderer dereferencing a null D3DX error
buffer after E_OUTOFMEMORY, and a d3dx9_29 tagged-pointer decode inverting
0x80011fab and dereferencing unmapped 0x7ffee068. Plenty of system RAM remained.
The original runtime reproduces E_OUTOFMEMORY in a standalone Win32/LAA GPU
fixture after filling low free virtual ranges and the legacy CRT heap.

LegacyShaderMemory installs BEFORE the first game effect, in CreateDeviceHook.
Signature-check d3dx9_29 compiler pointer decode and allocator calls, and verify
all import targets. Redirect ONLY that module's CRT/new/delete/process-heap and
VirtualAlloc/Free imports. Keep a fixed 128 MiB heap and 64 MiB page arena below
2 GiB, reserving upfront and committing as needed; never fall back to a high
pointer for these tagged data. Preserve pre-install allocation ownership and
synchronize arena reuse/decommit. The game and other modules retain normal
allocators. Keep these resources and hooks for the process lifetime; do not
unload the client while D3DX objects are alive. This does not change the game
executable on disk, swap DirectX versions, or fake a successful shader result.

LegacyShaderMemoryTests covers signature rejection, allocation ownership,
large buffers, zeroing, overflow/exhaustion, realloc, reservation/decommit reuse
and concurrent callers. LegacyShaderMemorySmoke is an explicit GPU fixture
requiring an owned d3dx9_29 path; it is NOT part of the portable CTest payload.
It can read original synthetic shader source, binary effects or a directory of
.fxo files. Never distribute the privately decoded BF2142 shader cache. The
stock pressure case fails; fixed source compilation and two complete batches
of 1,659 simultaneously held native effects pass. Win32/x64, 58/58 CTest and
13/13 render fixtures pass. The owner subsequently reported substantially more stable live VR loading.
Preserve v28 as fallback, including its weapon exterior archive and walker pack.

## Preserved BF2142 v28 behavior

The owner called v27 generally good, but reported reversed finger flexion, free
left-hand poses inheriting knife animations, and sickening ladder/parachute
camera movement. They also requested a more interesting lobby. v28 addresses
those together; the owner subsequently gave general positive headset feedback. Do not claim that
unit/GPU coverage establishes comfort or the feel of climbing in the headset.

Preserve v27 native snap turning: deliberate yaw enters the exact local look
caller once per input sample and is separate from actual recoil. Never restore
an independently accumulated XR turn reference. Shoulders/elbow poles, hands,
headset and native minimap share the level native input frame. Keep accepted
KnifeInGrip (+Z blade), right firearm bindings, rifle support, pistol cupping,
body-slot draws, empty-grip/no-recall policy and head/controller movement choice.

PoseEmptyFingers uses +controller-palm X for left inward flexion and -X for
right. Captured native anatomy verified the earlier sign was reversed. A free
left arm uses the canonical settled handReference/palm, even with a firearm
in the right hand, so slash/reload skeleton changes cannot replace its pose.
Held right-hand geometry is unchanged. These are controller touch poses, not
bare-hand finger tracking. Missing controller tracking retains existing fallback.

ReadNativeTraversal owns the read-only local traversal profile. Controlled
object comes from player weak +0x80, soldier from +0xcc. Recognize exact instance
and template vtables for LadderContainer and stock named parachute only; pods
share the latter class and must remain excluded. The ladder container moves
with the climber: native update +0x19f6b0 copies its matrix, updates translation,
and submits it through matrix setter slot +0x80 at +0x19fa70. The static ladder
at container+0x45c is a different object. Camera policy follows container/chute
translation and keeps its entry orientation level; native seat pitch/roll/bob
never become tracked head motion. Use recent on-foot comfort position at entry.

TraversalControls uses focused, fresh samples and read-only native feedback.
After velocity < -4.5 m/s for 180 ms, it releases then pulses native Space so the
game can deploy the canopy under its normal rules; ordinary jumps do not force
an immediate parachute. Ladder grip/pull accumulates bounded distance debt,
consumed by actual vertical movement. One active grip avoids double counting;
release stops. This drives native ladder locomotion, not a new rung/hand IK
system. Stick up/down is independent of HMD/controller movement heading while
mounted. Holster/ADS/motion/physical-stance interactions are gated off on mounts,
held state survives mounting, and reset/recenter clears gesture state.

MenuRoomGpu optionally loads LobbySceneFile. ExportLobbyScene.py reads owned
EU/PAC walkers, restores rigid mesh-part transforms and exports local BGRA
textures. No proprietary pack or decoded artwork belongs in the public repo.
The pack loader validates dimensions/counts/indices/normals/file boundaries.
Scene textures survive on CPU across device resets; own MSAA color/depth and
all native state are restored. Default no-asset room remains a fallback. Logo
and SteamVR branding still use the separate optional registration helper; never
initialize OpenVR inside the OpenXR presenter.

Validation: Win32/x64 built, 57/57 CTest, 13/13 GPU fixtures. Final walker scene
visually reviewed, about 3.2 ms/1280x720 stereo pair with GPU vertex processing
including readback. No live v28 gameplay/headset run was done. v27 is the saved
fallback; v23-v26 also remain unchanged. Completed v25 weapon archive remains.
Grenade guide visibility, knife transfer, manual reloads, networked remote IK
and new vehicle cockpits remain future work. Private checkpoint only; public
release packaging is a separate task.

## Start here

Read these sources in this order:

1. `README.md` for player-facing scope and requirements.
2. This handoff for the current architecture and invariants.
3. `docs/DEVELOPMENT.md` for reproducible build and test commands.
4. `devREADME.md` for the long-form history and evidence behind BFVR.
5. The relevant source and tests for the subsystem being changed.

The separate public
[Battlefield 1942 reverse-engineering repository](https://github.com/JayBiggsGMG/bf1942reverseengineering)
contains decompilation, memory mapping, and broader engine research. It is a
useful starting point for new game-internal work, but it predates some BFVR
discoveries. Treat it as supporting evidence, not as a newer source of truth
than this repository's current code and tests.

## What is included

The BFVR repository contains:

- Complete BFVR C++ source for the Win32 game-side components and x64 OpenXR
  presenter.
- Pinned OpenXR, MinHook, and d3d8to9 dependencies needed by the build.
- Player artwork and menu assets.
- Deterministic native tests and diagnostic probe targets.
- Player-payload staging and Inno Setup installer definitions.
- MIT license and third-party notices.

It intentionally does not contain Battlefield 1942, BF42++, private test game
installations, or the full outer reverse-engineering workspace.

## Process architecture

BF1942 is a 32-bit game, while current PC OpenXR runtimes are commonly 64-bit.
BFVR therefore uses two cooperating processes:

```text
BFVR.exe (Win32 launcher)
  -> starts suspended BF1942.exe
  -> selects one BF42++ loading path
  -> injects BFVRClient.dll
  -> resumes and follows the game process

BF1942.exe + BFVRClient.dll (Win32)
  -> redirects D3D8 through BFVRD3D8To9.dll
  -> renders stereo eyes and UI
  -> publishes shared D3D9Ex/D3D11-compatible textures and control state

BFVRPresenter.exe (x64)
  -> creates the OpenXR 1.0 instance/session
  -> consumes the shared textures with D3D11
  -> submits world projection, UI, menus, scope, vignette, and other layers
  -> samples OpenXR controllers and publishes normalized input to Win32
```

The cross-process protocol is pointer-free and versioned. Do not place native
pointers, C++ object ownership, or architecture-sized fields in shared records.

## Major source areas

### Launcher and compatibility

- `src/loader/BFVRLoader.cpp`
- `src/loader/Bf42PlusPlusCompatibility.*`

The launcher owns player startup, diagnostics defaults, BF42++ selection,
game-build reporting, injection, process replacement following, startup-movie
suspension/restoration, and launch-local renderer isolation.

### Win32 game client

- `src/client/BFVRClient.cpp`
- `src/client/D3D8StereoPairProbe.cpp` and `src/client/internal/`
- `src/client/ControllerInputOverlay.cpp`
- `src/client/BF1942FrameLimiterOverride.*`

The client owns game-side hooks, stereo world/UI production, recovered
Battlefield input submission, native arm/weapon integration, native BfMenu
feedback dispatch, confirmed local-kill observation, frame-limiter override,
and shared producer state.

### D3D8 translator

- `third_party/d3d8to9-1.15.1/`

BFVR carries a pinned and modified d3d8to9 translator. BFVR-specific changes
are documented in that directory's `BFVR_PATCHES.md` and `bfvr_*` source files.
The translator supplies D3D9Ex shared targets, packed depth, current
post-Reset presentation dimensions, and BFVR runtime diagnostics.

### x64 presenter and OpenXR

- `src/presenter/BFVRPresenter.cpp`
- `src/presenter/SharedTextureConsumer.*`
- `src/presenter/SharedTexturePerformanceSummary.*`
- `src/openxr/OpenXRPresentation.*`
- `src/openxr/OpenXRPerformanceSummary.*`
- `src/openxr/OpenXRQuickMenu.*`

The presenter owns OpenXR lifecycle, swapchains, D3D11 consumption/effects,
desktop mirror, controller actions, haptics, overlapping kill-sound voices,
and composition-layer submission.

`src/diagnostics/PerformanceSummary.h` supplies the common opt-in aggregate
timer. `BFVR_PERFORMANCE_SUMMARY=1` is a targeted, low-volume performance mode:
it leaves broad `BFVR_DIAGNOSTICS=off`, admits only prefixed x86 performance
summary messages, and adds QPC timing around existing x64 source/OpenXR calls.
It must not become a scheduling switch. In particular, it may not change the
runtime-timed request sequence, source ownership acknowledgement, swapchain
copy order, or `xrEndFrame` placement.

### Settings and menu

- `src/settings/UserSettings.*`
- `src/stereo/SettingsMenuInteraction.*`
- `src/client/SettingsMenuArt.*`
- `assets/SettingsMenu/`

`UserConfig.txt` is versioned, documented, and written atomically. Installer
updates preserve an existing user file. New keys require a safe default,
decode/encode coverage, menu wiring where applicable, and persistence tests.

`BFVRVersion.inc` is the canonical release version. C++ consumes it through
`src/BFVRVersion.h`; the loader resource and Inno Setup definition include it
directly. Update that file instead of independently editing launcher text,
Windows file metadata, installer naming, or the VR Settings credit. The credit
is a static transparent OpenXR quad below the unchanged 1024-square Settings
surface, so it does not alter the existing pointer coordinate system. The
desktop mirror reproduces the same separate quad.

The repository `assets/` directory is the canonical source for every runtime
asset. The CMake runtime-asset targets execute whenever `BFVRClient` or
`BFVRPresenter` is requested, so an asset-only edit is copied even when native
compilation is already up to date. Player staging copies directly from that
canonical directory rather than trusting a potentially stale build-tree copy.
The outer checkout's development BAT may mirror those files into its local
payload, but release publication still requires the complete release procedure
below.

The potential 1.0.2 settings group adds these runtime invariants:

- `show_arms` is a migration-compatible three-state visibility setting:
  `arms_and_hands`, `hands_only`, or `no_hands_or_arms`. Hands Only retains
  only game-selected mesh templates with explicit left/right-hand names;
  combined and unrecognized meshes fail closed to hidden. Tracking, native
  animation, controller weapon transforms, hand placement, and elbow solving
  continue while draws are hidden. Do not treat a narrow projection alone as
  ownership during an active or entering scope: magnified world soldiers
  satisfy the same lower bounds, so suppression must fail closed there.
  The global forwarding hook records only the current AnimatedMesh context;
  template-name classification is deferred until the draw policy has proven a
  first-person candidate in Hands Only mode, then cached by template/name
  storage identity. Do not move string inspection back onto every world-mesh
  draw.
- Hand weapons, mounted guns, and controller-pointer knife/throwable/gadget
  items each own an independent `WorldCrosshairMode`. Crosshair color is a
  shared base tint inside the existing D3D8 per-eye renderer; do not move this
  path into an OpenXR overlay merely to isolate it from grading.
- `hapticDeathSequence` and `localPlayerLifeState` share the verified
  local-player observer. Shared protocol version 22 lets the presenter start
  a bounded death effect and cancel it on a verified respawn.
- Movement and death comfort are two targets of one `OpenXRComfortVignette`.
  Death styling takes priority inside that compositor, which remains above
  the stereo world and below Ref2/interface layers.
- Color profile, exposure, contrast, and saturation are fused into the final
  world scaler shader after AO/SSGI/reflections/bloom. Both world eyes always
  retain that pass for live changes. Ref2 is passed neutral color settings;
  separately composed UI never enters the shader.
- Shared protocol version 22 adds a producer-to-presenter confirmed-kill
  counter and presenter-to-producer menu highlight/OK/cancel counters. The
  x86 client alone retains game pointers and invokes prefix-verified
  `BfMenu::playLoadMenu*` wrappers; the x64 presenter alone owns XAudio2 and
  creates an independent source voice for every confirmed kill.
- Remote-MP kill eligibility comes from the unchanged
  `GameClient::handleGameEventManagerEvent` path: event type `0x2A`, subtype
  3, distinct killer/victim IDs, and a killer resolved by native `findPlayer`
  to the BFPlayer returned by PlayerManager's native current-player virtual
  service at `+0x20`; `GameClient +0x170` is fallback-only. MP playback is
  owner-confirmed. SP did not traverse that received-client boundary, so BFVR
  now also observes prefix-verified retail `GameServer::handleScore` at
  `0x004AD2D0`. It accepts only score type 3 when the scoring BFPlayer is the
  PlayerManager current player and the victim is different/non-null, rejects
  teamkill type 6, and deduplicates only the same killer/victim pair if a
  listen server exposes both sources. A separate 300 ms first-kill-wins burst
  gate collapses grenade-style simultaneous kills into one sound; kills after
  the window may still overlap. Owner testing confirms correct SP and MP
  playback, teamkill silence, and multikill grouping. Other score variants
  remain silent. The custom WAV
  follows Windows output volume, not an unverified approximation of
  Battlefield's private master-volume value.

## Critical compatibility invariants

These are established behaviors, not cleanup opportunities.

### BF42++

- BFVR requires compatible BF42++ but does not bundle it.
- A separate installation exposes `bf42++.dll`; BFVR injects that library
  before its own client.
- Some packages bundle BF42++ as a DirectSound proxy named `dsound.dll`.
  BFVR recognizes the proxy structurally and leaves it on its natural loading
  path.
- If both forms exist, the recognized bundled proxy wins and BFVR must not
  inject the second DLL.
- Obsolete BF42Plus 1.3.4 remains a distinct blocked security case.

### Battlefield executables

- Unknown executable hashes produce a warning, not a launcher block.
- Game-internal features must locate supported code through evidence-backed
  signatures or validated relationships and fail closed when evidence does not
  match.
- Do not generalize one address across builds without matching evidence.

### Renderer packages

- Community installations may preload dgVoodoo or DXVK/Vulkan D3D DLLs.
- BFVR redirects the game to its pinned translator and isolates the translator's
  D3D9 dependency from package-local `d3d9.dll`.
- Do not rename, overwrite, or permanently remove a package's renderer files.
  Ordinary non-VR launches must remain unchanged.

### OpenXR

- Request OpenXR API 1.0. Newer headers do not justify requesting a newer API
  version; SteamVR and VDXR compatibility depends on the 1.0 request.
- `XR_KHR_D3D11_enable` is the mandatory graphics extension.
- The runtime selects the HMD adapter and swapchain sizes. Battlefield desktop
  resolution is not a hard-coded eye resolution requirement.

### Shared rendering and pacing

- Use the translator's current post-Reset presentation dimensions rather than
  stale initial CreateDevice dimensions.
- Keep producer ownership until the x64 consumer's queued source work is safe.
- Preserve the Oasis compatibility invariant: the x86 producer temporarily
  opens the first newly allocated D3D9Ex legacy shared texture through D3D11 on
  the exact producer adapter before publishing any texture handles. Do not move
  this primer to x64, remove it, or publish first.
- The established immediate path queues legacy shared-resource reads before
  `xrEndFrame` and defers their GPU completion/producer acknowledgement until
  afterward. Do not restore the former serial wait ahead of `xrEndFrame`.
- Do not add a SteamVR speculative future-frame buffer. Both the blocking and
  non-blocking `predictedDisplayTime + predictedDisplayPeriod` variants were
  live-rejected as equally awful and were completely removed. SteamVR, Oculus,
  and VirtualDesktopXR all retain the established immediate path.
- Do not add a native BF1942 bubble frame by acknowledging the current source
  before `xrEndFrame` and waiting for the next real request at a later Present.
  That distinct non-predictive experiment was also live-rejected: performance
  was the same or probably worse, and gun alignment gained a severe erroneous
  dependence on look direction. Its flag and implementation were fully
  removed. Preserve request-bound camera, weapon, and submitted-source cadence.
- UI capture policies distinguish full replacement frames from accumulating
  HUD draws. Combining them blindly can create trails or repeated translucent
  elements.
- Retail projected terrain shadows have two proven outer producer returns,
  `0x00682E95` and `0x00683ADD`, under wrapper `0x0066800A` and shared
  `PatchCellBlock::draw` return `0x0069922E`. Keep both in the fail-closed
  static classifier. Runtime state discovery is fallback-only and may inspect
  only that shared indexed terrain-cell submission; do not spend its budget on
  unrelated alpha-blended perspective draws.

### Frame limiter

- BFVR disables BF1942's internal `lockFps` value only inside the BFVR-launched
  process.
- The override is signature-backed, validates the live owner/value memory, and
  periodically reasserts `-1.0f` from the Present path.
- Do not edit `VideoDefault.con`; that would affect ordinary flat launches and
  could leave user files altered after a failure.

### Diagnostics

- `BFVR_DIAGNOSTICS=off` is the player default and must have no periodic file
  logging or expensive proof collection.
- `normal` enables bounded development summaries.
- `deep` enables expensive evidence probes for focused investigations.
- Required rendering, state restoration, and compatibility work is not a
  diagnostic and remains active when diagnostics are off.

## Controls

OpenXR input is normalized in the x64 presenter and converted to recovered
Battlefield logical input in the Win32 client. Surface vehicles and aircraft
are separate control branches.

The default aircraft layout is:

- Left stick: throttle and roll.
- Right stick: pitch and yaw.
- `Aircraft Pitch + Roll on Same Stick`: off.
- `Swap Aircraft Sticks`: off.

The two aircraft options apply only to the recovered `VCAir` category. They do
not change infantry, ground vehicles, boats, turrets, or mounted weapons.

Surface/sea physical controller-motion aim is proof-gated by the occupied
`PlayerControlObject` weapon vector used by BF1942's native vehicle HUD.
Only a valid non-empty vector enables motion aim. Empty, malformed, unreadable,
or unavailable vectors fail closed with controller motion disabled; head look,
right-stick `mouseLookX/Y`, keyboard, and mouse remain independent and active.
Every current-control-object identity change resets motion history, so moving
between unarmed and armed seats cannot submit a stale controller delta.

SteamVR binding changes are runtime-local. Meta OpenXR and VDXR use BFVR's
suggested OpenXR bindings, not a user's SteamVR-only customization.

Menu controller pointers share `UiPointerSmoother`, a normalized 2D adaptive
low-pass with a 0.0015 output deadzone. The Quick Menu maintains independent
history for its main, command, and utility surfaces; VR Settings and native
BF1942 menu injection each maintain their own history. Rendering, hover, drag,
and activation always use the same filtered coordinates. Reset history on
tracking/panel discontinuities and keep the saved default-on toggle live; do
not move this policy into startup, OpenXR runtime selection, or Oasis setup.

All 3D crosshair controls are grouped on Controls page 2. The opacity setting
is strictly 5..100% in 5% steps and changes the premultiplied ARGB tint used by
both the world aiming crosshair and hit marker. The native scoped RGB synchronization
remains color-only.

## Currently validated compatibility

Base-package tests include:

- The development/HD Battlefield installation.
- An Anthology non-Vulkan/dgVoodoo installation.
- The Moongamers dgVoodoo package with bundled BF42++ proxy.
- An Anthology VK/Vulkan installation with separately installed BF42++.

Runtime tests include:

- Meta Quest Link / Meta OpenXR.
- SteamVR OpenXR.
- Virtual Desktop through VDXR.
- Virtual Desktop through SteamVR mode.

This matrix proves those tested combinations only. It is not permission to
claim that every executable, headset, controller, mod, or runtime works.

## Build and test

Follow `docs/DEVELOPMENT.md`. The required acceptance floor for a source change
is:

1. Configure and build the Win32 tree.
2. Run the complete Win32 `ctest` suite with zero failures.
3. Configure and build the x64 presenter tree.
4. Run targeted no-HMD probes when the changed subsystem has one.
5. Perform a physical headset test for presentation, controls, runtime, or
   package-compatibility changes.

Build success does not prove headset behavior. Record exactly which package,
OpenXR runtime, headset, and test path supplied a compatibility result.

## Release procedure

For a release candidate:

1. Update version metadata, `CHANGELOG.md`, player documents, and installer
   definitions.
2. Build clean Win32 and x64 release artifacts.
3. Run all automated tests.
4. Stage only `installer/player-manifest.txt` through
   `tools/Stage-BFVRPlayer.ps1`.
5. Build the unsigned Inno Setup installer.
6. Install that exact installer into a clean supported Battlefield copy.
7. Test startup, menus, single-player, multiplayer, firing, settings, VR
   shutdown, uninstall, and preservation/removal of user configuration.
8. Publish the exact tested installer and its SHA-256 checksum with the matching
   source tag.

Never substitute a rebuilt binary after installer testing without repeating
the installer test.

## AI-agent workflow

An AI agent continuing this project should:

1. Inspect `git status` before editing and preserve unrelated user changes.
2. Identify the owning subsystem and read its tests before modifying code.
3. Separate observed evidence from interpretation.
4. Use pure helper policies and deterministic tests for new routing, math, or
   compatibility decisions.
5. Prefer the smallest reversible change that preserves the invariants above.
6. Update this handoff when a confirmed result changes an architectural rule.
7. Update `CHANGELOG.md` only for user-visible behavior.
8. Never claim physical compatibility from static analysis or compilation.

When historical notes conflict, use this priority order:

1. Current passing source and tests.
2. Current logs from a controlled test.
3. This handoff and `docs/DEVELOPMENT.md`.
4. `devREADME.md` historical record.
5. The older external reverse-engineering repository.

## BF2142 development branch

See [port status](BF2142_PORT.md), [camera findings](BF2142_CAMERA.md), and
[native hands findings](BF2142_HANDS.md). The user confirms v11 headset gameplay,
controller hands, floating HUD and OBS recording are working well. Reported
bugs: ineffective laser clicks, invisible controller-opened pause menu, shot
misalignment, unstable left-hand pose, flat ADS and poor recenter discoverability.
The subsequent v12 headset run confirms the menus and gun-directed shots work.
The user reports a bad EU assault rifle left grip, rejects the circle optic/
removed ADS action, and reports a crash possibly while firing. v13 addresses
that follow-up; preserve v12 as a fallback and do not restart an active match.

Preserve these invariants:

- BF2142 has a separate native D3D9 launcher/client. Never apply BF1942 offsets
  or load BF42++ into BF2142. Match signatures and native identity before use.
- Both eyes use one runtime request; second Render receives zero delta. Restore
  native cameras. Game simulation/input remains outside the pair.
- Suppress internal Present until eye readback, then present the completed pair.
  Do not read DISCARD after Present or desynchronize native graphics caches.
- Renderer slot 15/RVA 0x2ea10 is a FIRST-PERSON WEAPON pass, not HUD. Actual HUD:
  exe 0x34e610, HudManager vtable 0x5a6b50 slot 7. Flash display boundaries:
  renderer 0x117a20/0x118af0. Earlier interface-boundary assumptions were wrong.
- Accumulate isolated UI batches, match MSAA, resolve outside BeginScene, restore
  target/depth/viewport/state after composition and release resources at Reset.
- NativeWorldActive means local-player lifetime, separately from camera
  eligibility. Missing eye pairs never authorize publishing the full world as
  UI. v12 allows explicit native Flash batches outside NativeRender while paused,
  with native cursor visibility; keep gameplay=true through that request path.
  It publishes black projection textures plus isolated UI, without recentering
  or recategorizing the player as login. Login/loading still use the UI panel.
- Shared textures remain owned until consumedFrameSequence; keyed-mutex success
  is S_OK, not every nonnegative result. BF2142's FullEyeTextureFov and
  OwnControllerMappings flags preserve BF1942 defaults and protocol-v23 layout.
- Controller samples match request sequence, predicted time and focus. DInput
  gameplay overlays expire after 150 ms and preserve physical input.
- MenuPointer preserves the accepted 1.6 m width / 1.5 m distance and shares its
  yaw-only anchor with frameUiWorldAnchor. Exclude aspect-fit padding. The beam
  is in the eye images and cursor in transparent UI.
- IMPORTANT correction to the previous handoff: native Flash mouse clicks use
  WINDOW MESSAGES, not the DInput overlay. Exe window listener RVA 0x1570 routes
  WM_MOUSEMOVE/WM_LBUTTONDOWN/UP to the Flash dispatcher. Escape is handled on
  WM_CHAR at RVA 0x1733. MenuWindowInput posts only to the foreground, owned game
  window; gameplay stays in DInput. Do not duplicate click/key edges across both
  paths. Release posted mouse state on loss, expiry or ray departure. A held
  trigger on entry must first be released; a held B that opened pause must not
  immediately close it. No global SendInput/button injection.
- Manual Home/right-stick recenter also works in menus; RecenterPolicy resets
  once after a sustained (750 ms) tracking/focus loss. It cannot infer headset
  removal if the runtime continues reporting valid focused tracking.
- MotionHands excludes the older, disabled-by-default TrackedWeapon camera
  workaround. Preserve first-person roots and native animation ownership.

### Native hands and firing

NativeHands uses accepted focused grip/aim poses with a 150 ms age limit. Only
local infantry, the recognized active GenericFireArm and verified 70-bone first-
person topology are eligible. Menus/recenter/tracking loss clear ownership.
Hook the 1P finalizer at exe 0x1ee980 after native animation. Modify final
matrices, not local source bones, IK handles, assets, camera bone 0 or root 1.
Native code binds bone 54 and mesh parts afterward. Keep the v12 right-hand
binding. HandBindingCache settles the off-hand separately: reject rearward/
implausible foregrip poses, require 250 ms of closely matching samples after
750 ms from equip, and reset the stability interval after a sample gap. Freeze
once settled so reload cannot recapture. Reset on weapon/soldier/skeleton
identity changes. No new per-weapon asset offset is assumed.

Correction to v11: fire entry 0x1f2bb0 takes THREE stack arguments, ret 12, and
returns a projectile pointer. The FIRST matrix is used by CreateProjectile
(0x1f18f0, ret 8) for the main shot. The SECOND affects a secondary projectile
path. The old hook changed only the second. v12 hooks the launch getter at 0x1f1a20 (interface slot +0xd4, ret 0) BEFORE
0x1f31d0 computes velocity. Return thread-local storage containing
nativeLaunch * inverse(nativeCameraWorld) * solvedWeaponWorld. Native code then
calculates muzzle speed * corrected forward + inherited player movement and
applies its normal clamp. Do not remap the main matrix again at 0x1f2bb0 or
rotate inherited movement. That constructor hook only substitutes the secondary
matrix and records an actual mapped shot. Keep native offsets/spread/creation;
never patch native fire state globally. Receiver/weapon identity, generation,
focus and age must all match. The user confirms v12 shots follow the gun.
v13 preserves this firing code unchanged.

The v12 generic red-dot circle was rejected by the user. v13 removes its draw
call and restores native right-mouse ADS from right grip even with MotionHands.
The game retains its zoom/accuracy state and native scope artwork; proper VR
scope rendering is still unfinished. Do not describe restored input as complete
VR ADS. The inactive WeaponOptic helper is not a player feature. Missing weapon
backs/stocks remain explicitly deferred, with no game asset changes.

### v13 renderer crash guard

v12 dump BF2142.exe.75700: AV reading 0x54 at RendDX9_ori+0x1264, ECX=0.
Call chain +0x4c40 -> +0xbc4c0 -> +0xbe9d1 inside NativeRender; different from
the older D3DX shader-loading crashes. +0x4c40 uses the native query-map find
(+0x5570) but treats an end sentinel/null record as a real object. The decision
at +0xbc4c0 is optional; its caller already has an ordinary draw branch on false.

NativeQueryGuard validates decision/lookup/update code, lookup calling convention,
relative call and relocated manager address, then detours only that decision.
Read id from object+0x350; manager is renderer+0x1f8e74, map at manager+0x10,
end sentinel at manager+0x14, node value +0x10, nil byte +0x15. Call the native
read-only find; absent/null data returns false, valid data forwards unchanged.
Zero is not categorically an invalid id. Do not broadly swallow rendering AVs,
patch game files, or disable the entire renderer. One missing-record log per
process, no per-frame dump. This guards the observed failure; record lifetime
and why that id was missing are not proven, and overall crash freedom is untested.

### Validation and continuation

v12: Win32 build, all 41 deterministic CTests, matching x64 presenter build,
renderer GPU smoke and stereo GPU smoke with/without 4x MSAA pass. Regressions
cover queued mouse/escape edges, held B entry, stale releases, recenter timing,
main launch-matrix rotation, animated grip stability, optic clipping/colors,
and 120 paused UI-only frames followed by resumed stereo without an extra
neutral capture. GPU fixtures substitute native game entry points; they do not
prove native game behavior. The subsequent v12 user run confirms menus/aim,
while v13 support placement and live crash behavior await the next combined run.

The user's latest v11 report supersedes earlier 'headset test pending' and 'OBS
unresolved' notes: gameplay/hands and recording were used successfully, with the
specific bugs above. Earlier intermittent native shader-loading crashes are
still unproven fixed; keep failure logging and throttled loading transfers.
Preserve the embedded, disabled input-transparent desktop preview. Never add an
automatic second recording window. Avoid per-edit requests to the user; stage
combined batches and preserve a runnable fallback.

For v13, all 41 CTests and both architecture builds pass; explicit renderer,
stereo and MSAA fixtures pass. Hand tests replay equip/settle/reload/gap behavior
and preserve the right-hand binding. ADS mapping again presses native right
mouse. A local mapped-renderer probe (no DllMain/game launch) exercised the real
query find and installed detour with absent/end/null records, valid nonzero and
valid zero keys, plus null owner/manager. Valid cases forward to a test callback
once; it does not execute the full native draw path or prove live stability.


## BF2142 v14: level infantry camera and recoil isolation

The user accepts v13 hand placement, but reports an ill-defined slowly rotating
world and severe head recoil while firing. Do not change the authored grip
cache, menu distance, OBS/single-window behavior or the native firing adapter.
v14 is staged for their next launch; no claim of headset comfort verification.

NativeComfort supplies a separate read-only tracking frame for local, alive,
unattached infantry. Native camera position/stance is retained, while absolute
orientation comes from soldier body yaw (+0x25c) plus local look yaw (+0x278),
minus an observed cumulative horizontal recoil contribution. These angles are
degrees. Native update 0x18a6d0 redistributes local yaw into body yaw (0x18ae82
through 0x18ae98 and 0x18aea4); using body yaw alone would drift during settling.
The camera builder 0x1890a0 additionally applies visual yaw +0x27c and pitch
+0x270/+0x274; those do not belong in the headset tracking frame. Level world-up
is built directly, with HMD pitch/roll/yaw applied afterward by StereoCamera.

Getter 0x186140 is a no-stack-argument thiscall returning float in x87 ST0.
The exact call at 0x18acf9, return 0x18acfe, adds its horizontal recoil result
to input before native heading integration. The hook returns that result
UNCHANGED. Only that caller on the verified local infantry owner contributes
to a bounded double-precision yaw correction. Other callers/players pass
through. Compensation follows the soldier weak-reference identity, survives
manual recenter/menu/pause, and resets on lost/changed owner. No input, recoil
configuration, projectile, skeleton root or native camera state is overwritten.

Signature checks cover the getter, caller/relative target, input update,
body/local redistribution, visual recoil additions and vtable relationships.
Invalid profile/owner/transform or a camera farther than 3 m from the soldier
falls back to the prior native camera path. This is infantry comfort; vehicle
comfort is not implemented. Animation skeleton mode is not a camera eligibility
condition: ADS must not silently re-enable head kick. All native camera
translations are still retained; this change isolates rotation, not every
possible positional camera effect.

NativeStereo pins the comfort source across an eye pair and still restores
both complete native RenderViews after each pass. NativeHands converts the
same comfort world frame into skeleton space for HMD/controller composition.
Bone 0 stays native, including the nativeCameraWorld used to extract firing
offsets in MapTrackedFire. The accepted HandBindingCache, SolveTrackedHands,
LaunchHook and FireHook behavior is unchanged. Do not replace the firing
reference with the comfort camera or apply recoil compensation twice.

Auto-recenter now distinguishes unknown controller packets from known runtime
focus: a coherent focus sample within 150 ms of the rendered frame can update
RecenterPolicy even across request boundaries; absent/stale packets cannot
start a tracking-loss timer. Gameplay/controller commands still require the
original exact request/time/focus match. Invalid HMD requests and real focus
loss retain automatic reset; Home/right-stick remain manual controls.

Verification: 42/42 Win32 CTests, x64 presenter, renderer GPU, stereo GPU with
and without 4x MSAA. Comfort tests simulate 20,000 fire/recovery/body-settling
frames, deliberate turning, wraparound, ownership resets, eye separation,
physical head motion and skeleton/world-frame agreement. GPU fixtures inject
animated source pitch while testing the real session/UI loop. A private x86
synthetic adapter probe verifies float-return ABI, exact caller/local ownership,
1,002 unchanged native returns and invalid camera/owner fallback. All new
profile signatures/relative target/vtable relationships also match the actual
installed EXE. Probes do not attach to the running game or prove live comfort.
Private logs/probes and v13 backups remain outside this source repository.


## BF2142 v15: deployment menu recognition

The user reports v14 is in a very good place and requests a fix for the laser
on spawn/class selection, plus a roadmap. Preserve the accepted comfort, hands,
shot direction, floating panels and single-window OBS behavior.

MenuPointer::MenuVisible now checks a read-only NativeMenuState before falling
back to CURSORINFO. Use this shared query for both the gameplay/menu controller
split and standalone Flash capture. Do not infer deployment from a null soldier:
loading, death cameras and transitions are not sufficient evidence of a menu.
Keep Windows game-focus checks and exact controller sample/focus checks.

On the supported EXE, current frontend global RVA 0x60f7a8 points at the in-game
Flash state with vtable 0x525350 and initialized byte +8. Window-listener mouse
move resolves that same global at 0x18f6 and calls slot 19 (0x874d0); left-down
calls slot 21 (0x874f0), left-up slot 22 (0x87520). This is the SAME window-message
route as existing working menus, not a reason to add duplicate DInput clicks.
Profile checks validate the global reference, initialization store, move/release
signatures and vtable relationships before reading state. Reads are guarded;
no writes, new detours or cached frontend-object pointers. Unknown layouts keep
the cursor fallback. The new state reader is BF2142-only.

v15 verification: both builds, 43/43 CTests, hidden renderer and real stereo/UI
fixtures with/without 4x MSAA pass. New native-state tests cover absent/inactive
state, deployment, replacement, spawn closure, unfamiliar vtables, invalid
pointers, signature mutations and failed reconnection. Every profile condition
matches the installed EXE; read-only inspection of the user's deployment screen
matches the new active predicate. This does not prove headset laser clicks.
No live game restart, injected probe, class change or spawn was performed.

The usual local desktop launcher selects v15 on next launch. Local before-v15
backup and Launch-VR-Previous-v14.cmd preserve the accepted v14 build/config.
Next substantial milestone: proper weapon-specific VR sights/scopes (aligned
optic, reticle visibility through the sight, magnification where applicable),
then reload/weapon-selection interaction, vehicle controls and comfort, then
stability/performance and Remaster compatibility. Missing gun backs/stocks stay
deferred as requested. Keep combined user runs instead of per-edit testing.


## BF2142 v16: first weapon-specific magnified optics

User authorized the next substantial optics batch. v15 remains the immediate
fallback; the accepted v14 fallback remains available. No game session was
restarted or mutated during this work. Headset v16 behavior is still unverified.

GunOptics contains exact-name stock profiles for eu_ar_rifle, as_ar_rifle,
eu_sni, as_sni and unl_adv_sni. Rear aperture position/shape was measured from
installed geom-0/LOD-0 meshes in private diagnostics. No proprietary meshes or
textures were copied into this repository. The independent private reader used
the layout documented by the bf2-blender project:
https://github.com/marekzajac97/bf2-blender/tree/master/io_scene_bf2/core/bf2/bf2_mesh
No external parser dependency or copied parser implementation is shipped.

The new read-only NativeHands export returns the SAME solved bone-54 world
frame used by the accepted firing adapter. It verifies local owner, current
weapon, focus and age. Optic images require the exact current input generation;
only the LOD policy may accept the previous sample within 100 ms, because zoom
updates can precede the next render request. No change to bindings, IK, launch
mapping, native recoil, comfort heading, recentering or menu input.

NativeOptics matches GenericFireArm +0x24 template (vtable EXE RVA 0x56c758).
Its name getter at 0xb7510 points to string +0xc: allocator, inline-16 buffer/
pointer, length and capacity (template +0x10/+0x20/+0x24). Bound and lowercase
ASCII names. Scan only the known component fields +0x1b4..+0x1e8 for the exact
DefaultZoom interface vtable 0x571360, base vtable 0x571438, owner base+0xc, and
template at interface+0x20 (vtable 0x571640). Template +0x18/+0x1c is the factor
array, +0x2c zoom LOD; interface +0xc is current zoom step and +0x24 fine offset.
Require LOD 1 and the measured stock first zoom factor (.484 or .312). These
name/factor checks are not a fingerprint of replacement art; Remaster remains
uncalibrated. Unknown/invalid layouts retain the prior behavior.

EXE 0x1da770 is DefaultZoom's visual LOD setter, thiscall(void*,int), ret 4.
Validate code, exit and base-vtable slot 17, plus factor/name/state accessors.
For ONLY the verified current supported tracked weapon, turn LOD argument 1
into 0, then call the original once. Do not change the zoom step, template,
accuracy, weapon firing or zoom-FOV getter. This keeps the normal physical gun
instead of BF2142's oversized flat aiming mesh. Native ADS still owns its
accuracy behavior. 2x assault/4x sniper are VR optical starting magnifications;
scale by firstFactor/(currentFactor-fineOffset), clamped 1..16. Do not describe
these starting magnifications as measured exact flat-camera magnification.

StereoSession captures the two ordinary eyes first, including their exact
camera matrices. If ADS is active and an eye is within the lens eye box, request
one scope replay at zero delta. IsScopeRender marks pass 2 and
IsSecondStereoEye now means any replay pass (>0). BeginNativeScope uses the
existing full RenderView save/restore mechanism; both views are validated before
writes. Its world camera looks from the lens toward a 100 m bore zero. The
first-person view's projection clips everything nearer than 4 m so the held
weapon does not fill the scope source. NativeHud's scoped replay is skipped;
StereoHudBegin never captures that pass. Keep the previous normal UI target
intact. The extra native Present is suppressed, so animation time still advances
once per frame, with only one completed desktop Present per stereo pair.

CompositeGunOptic intersects each eye ray with the actual lens plane, clips to
its ellipse/rounded rectangle, uses per-eye finite-zero parallax and fades at
eye-box/eye-relief boundaries. No generic ring or full-HMD magnification.
EyeRestore uploads the normal composited left eye and restores every D3D state,
render target and depth binding before ordinary UI/desktop presentation. No
extra desktop window. Clear its resources on device Reset. A replay/resource
failure disables the optics adapter; a failed desktop restoration ends stereo
rather than presenting a telescope image as the headset view.

Known limits: the CPU lens composite does not consult depth, so hands/nearby
occluders can be overdrawn inside the aperture. Native game scope replay and
physical lens fit need one combined headset run. The extra full-source-size
native render/readback while aligned costs performance. No asset completion,
vehicle optics, multiplayer or Remaster compatibility claim. WeaponOptics=0
keeps v15-style native ADS; default is 1 when motion hands are enabled.

Verification: 44/44 Win32 CTests and x64 presenter build; renderer and stereo
GPU fixtures, with/without 4x MSAA; added scope fixture with 180 render calls but
60 animation advances, normal desktop background after scope restore, 120
UI-only pause frames, resume without recenter, and clean separate HUD. Optics
unit tests cover all profiles, 2x/4x fields, eye relief/alignment, invalid poses,
aperture clipping, reticle colors, rigid-frame invariance and shared 100 m zero.
Every new native profile condition matches the installed EXE. A private x86
synthetic probe verifies one original LOD call, exact owner, unknown/stale/fault
fallback, unchanged zoom state and fine zoom. These do not prove live gameplay.


## BF2142 v17: deployment GUI, automatic ADS and local mesh repair

User feedback supersedes the v15/v16 verification boundary above: v16 optics
are accepted in-headset, but grip ADS changed the hand shape, missing gun backs
were visible, and the deployment laser was still absent. v15's EXE+0x60f7a8
frontend is Flash, not the separate native HUD's deployment screen.

NativeHudState validates the HUD mode getter EXE+0xc2db0, isInMenu query
+0x3515c0, their HUD vtable slots and renderer Render slot. Resolve renderer
+0x1f8e58 (vt +0x1c1868), renderer object +0x5cc HUD (EXE vt +0x5a6b50),
visible +0x268, mode +0x270. Recognized modes 0..27 exclude 0/2/10/11 as in
native isInMenu; this path does not use its comms-menu flags. Root +0x84 has
vt EXE+0x5b8528; dimensions +0xc/+0x10 must be finite 100..8192. Root +0x20
pointer has vt +0x5cb870 and enabled byte +0x10. Re-read owners every time.
Active or unrecognized Flash ownership blocks HUD clicking; a recognized
inactive allocated Flash object does not. Do not infer menus from death.

NativeHudPointer hooks pointer Move at EXE+0x46dcd0, thiscall bool(pointer,
root,float x,float y), and root Update +0x3c9160, bool(root,float dt). Match
root/pointer against the fresh HUD target, scale ray UV by native dimensions,
and enqueue down via +0x46e2a0 (root,1,1,1) or up +0x46e2b0 (root,1,1).
These dispatch through pointer vtable +0x4c/+0x50, +0x46dd20/+0x46dd50, into
the GUI event queue. Validate prologues/vtables; cache the untouched HudState
profile BEFORE installing Move's detour. Initialize call targets before hook
enable. Process button edges once per published serial; replayed eyes do not
click twice. Input expires at 150 ms and requires foreground ownership. Never
release through a stale root after a map/owner change. Existing window-message
Flash routing and fixed panel geometry remain. Native HUD keeps DInput Escape.

AutoAdsPolicy uses the same MakeOpticView visibility as scope composition:
alignment >.55 for 180 ms enters, <.15 for 300 ms exits, grip enters immediately.
A >250 ms gap resets. Native cancellation blocks reacquisition until alignment
<.15 and grip release. NativeOptics validates the SetZoom bridge EXE+0x1da610
(interface vt slot 39), relative call target, +0x1da3f0 entry and +0x1da52c
ret 8. Calls are thiscall(zoomBase,int step,bool immediate=false); the native
routine owns its gameplay/event effects. This v17 path supersedes v16's
read-only zoom-state invariant; the separate v16 LOD detour is still visual
only. Release only zoom engaged by this adapter. Revalidate current local
inventory before release; preserve keyboard-owned zoom and native fine zoom.
Use the current tracked generation to engage; unsupported items retain native
input. Request once per runtime sample and update after both normal eyes,
before optional scope rendering. Diagnostic fixtures never mutate game ADS.

AdsHandPose preserves settled non-ADS bones 2..53 relative to native gun 54
through ADS and its 350 ms exit blend, before the accepted tracked hand solver.
Do not change camera/root 0/1, native gun parts 54..69 or firing code. Reset on
owner/skeleton/equip change; bindings must not settle during supported ADS.
The current behavior may need exit/reload animation timing tuning in-headset.

scripts/bf2142/RepairWeaponMeshes.py is asset-free and writes a NEW archive.
It handles the five calibrated stock optics, normal FP LOD 0 only: duplicate
solid triangle winding with flipped normals, retain UV/bone data, skip alpha
materials. For the PAC assault rifle, clip rigid part-0 third-person geometry
behind the FP back plane, interpolate attributes and append its stock material.
Part count stays unchanged. Verify all other LODs structurally identical and
ZIP entries intact/CRC valid; respect uint16 relative-index capacity. Private
original/repaired archives, renders and backups remain OUTSIDE this repository.
This is not a guarantee for replacement art: local launch/install tooling must
never silently overwrite an unrecognized newer archive or Remaster install.

v17 verification: both builds, 47/47 CTests, five mesh-tool tests, installed
binary profile checks, hidden renderer/stereo/scope fixtures with/without 4x
MSAA. Hidden-window Present may successfully return S_PRESENT_OCCLUDED; the
fixture accepts that or S_OK and still checks every actual call with SUCCEEDED.
Preserve v16's 180 renders/60 advances, normal eye desktop restoration and
pause/resume/no-extra-recenter invariants. v17 has NOT been tested in-headset:
real HUD class/spawn selection, ADS feel and repaired textured art remain to
confirm. Source changes preserve BF1942 and the x64 presenter behavior.


## BF2142 v18: fix v17 transparent-index corruption; restore v16 hands

User supplied a v17 screenshot of displaced hands and cyan additive triangles
stretching from the rifle. The mesh defect is reproduced offline: the v17
parser/writer retained only material inum indices for alpha mode 1 even though
alpha_sort is 8. Native loading selects istart + direction * inum, so the next
seven views read different materials' indices or exceed the buffer. A structural
round-trip through the same incomplete parser could not detect the loss. The
previous untextured solid-face render also excluded the affected materials.

RepairWeaponMeshes now reads/writes alpha_sort full face lists, validates every
local index and full draw range, and refuses missing/mismatched lists on output.
Transparency mode 2 remains one index list. Opaque repairs alone add faces; all
transparent vertices, material fields and all direction lists remain exact.
The external-format fixture is hand-packed independently of Mesh.encode and
places an eight-order blend draw between opaque draws. The saved v17 writer
fails that fixture. Test late-list out-of-range indices and missing sets too.
Format cross-check: the upstream bf2-blender MaterialWithTransparency loader
also indexes istart + direction * inum:
https://github.com/marekzajac97/bf2-blender/blob/master/io_scene_bf2/core/bf2/bf2_mesh/bf2_visiblemesh.py
No upstream code or proprietary assets were copied into the source tree.

Rebuild from the untouched BEFORE-v17 archive, never the corrupted v17 output.
The private v18 comparison confirms 10 unchanged transparent materials/80 lists,
all protected LODs and 353 unaffected ZIP members. Keep original backups. The
local selector accepts the known v17 hash only as an input to migrate away from,
not as a selectable repair. v16 rollback restores the original archive, and
v18 applies the corrected one. Unknown archives still fail without replacement.

Hands: disconnect AdsHandPose.cpp from the client and restore NativeHands::Apply
to the accepted v16 implementation (verified equivalent except comments and
whitespace). Remove conditional binding capture and the cached full-arm/finger
replacement. Leave the experimental helper/test historical, never re-enable it
based solely on its synthetic tests. Preserve NativeHands' local-weapon export
used by the opt-in auto-ADS adapter. AutomaticADS defaults to 0 in VrSettings,
its loader, sample INI and installed recovery config. Right-grip native ADS and
accepted v16 optic rendering are the default again. AutomaticADS=1 remains an
experiment; no live isolation proving the sole hand-regression cause exists.
Native deployment input, shot direction, comfort and recording are unchanged.

The next real play session must establish v18 appearance and hand feel. The
user's v17 headset regression supersedes v17's automated success; do not call
v17 a working fallback. Use the accepted v16 launcher with original assets.

V18 final local verification: both builds, 47/47 Win32 CTests, 8/8 mesh tests,
all five renderer/stereo/scope/MSAA GPU fixtures, launcher inspection and
installed archive SHA256 passed. Checkpoint: private local/stereo-v18-checkpoint.json.
No new headset playthrough was performed.


## BF2142 v19 asset coverage; native runtime remains v18

User said the incomplete surfaces looked reverted. Live inspection on 2026-09-23
proved BF2142VRClient.dll came from port-x86-v18, and Weapons_client.zip SHA256
matched v18. The current GenericFireArm template was eu_mg. This was a coverage
gap: v17/v18 edited only five optic rifles and only ordinary FP LOD0. A specific
follow-up question about whether the assault rifle is also affected may still
need the user's answer; do not claim all reported surfaces are proven fixed.

Expand RepairWeaponMeshes.NAMES to 28 firearms/underbarrel variants. Audit of
the owned base meshes found 54 total first-person LODs (handguns have one), all
compatible with the checked declaration/format. Repair every group-0 LOD;
protect groups 1+ (world), all transparent materials and all source vertices/
indices as exact prefixes of repaired opaque draws. Never copy world geometry
with mismatched rigid part IDs. PAC rifle part-0 rear-stock addition remains
reviewed separately. Other genuinely absent components still need real art
completion; reversing triangles cannot create missing geometry or new textures.

Nine tests pass, with a new normal/ADS machine-gun fixture. Compare 28 outputs
against the untouched original: 50 transparent materials / 400 sort lists are
exact, 330 unrelated ZIP members exact, all world LODs exact. Confirm client
source hashes match the v18 checkpoint. This update does not rebuild or rename
the v18 runtime: main launcher uses port-x86-v18/baseline-x64-v18 and selects
asset revision v19. v18 prior-assets and v16-original fallbacks are retained.
The game was running when staged; the selector applies only after it exits.
Do not mark the new archive installed until the game archive hash matches.


## BF2142 v20 actual surface completion, using v18 runtime

The v19 archive was confirmed installed (new process and correct file hash).
User clarified that all guns omit the sides hidden in traditional first person,
then provided a screenshot and 69.1 s VP9 clip. Extracted frames show rear/
underside openings during physical rotation. v17-v19 duplicated existing faces;
that cannot reconstruct an omitted outer wall. Do not describe v19 as a full
model completion. The new v20 update adds real world-model donor surfaces.

CompleteWeaponSurfaces.py builds a per-rigid-part BVH of original, non-reversed
opaque FP triangles. For each opaque world triangle, cast a short segment along
its geometric normal and accept coverage only for the same part and matching
outward normal (dot >= .35); the opposite wall seen through a hollow gun cannot
count as coverage. Sample corners/centroid, subdivide mixed coverage to depth 4
or 6 mm edges, then keep uncovered pieces. Preserve interpolated UV/normal/
tangent data and part IDs. Appended pieces inset 0.5 mm along vertex normals;
original v19 materials remain exactly intact. Split added draws before 30,000
vertices so subsequent backface duplication stays in uint16 index capacity.

Use only original world group 1 / LOD0, and append to FP group 0 / LOD0. The
v19 alternate aiming-mesh backface repair is retained; do not graft unaligned
world geometry into the special aiming meshes. Complete before reversing
faces, never query coverage against previously reversed triangles. Expand FP
bounds but retain original part count. The existing reviewed PAC stock graft
runs first and is included in the coverage tree, avoiding another full copy.

Reviewed exceptions: unl_har_rifle/unl_har_rocket world positions require scale
(1.6,1.2,1.2), translation (0,.006,.03) in the FP frame, measured from common
moving-part bounds. Transform normals with inverse scale, tangents with scale.
PAC as_handgun world part 2 is the FP bolt part 3, not its small trigger part 2.
Skip any other world part absent from FP. This currently excludes 160 donor
triangles for the extra world-only Voss launcher part; no guessed attachment.
Do not claim every possible accessory has been reconstructed or every animation
verified. Existing gun/wrist/firing transforms and controls are unchanged.

Validation: 9 existing mesh tests + 6 surface completion tests. The latter
checks a real opposite wall at a distinct position, preservation of the already
covered front, normal direction, UV/part ownership, unknown-part exclusion,
transparent donor exclusion and the explicit transforms. Original v19 archive
comparison proves every old material exact, every alternate/world LOD exact,
50 transparent materials/400 sort lists exact and 330 unrelated entries exact.
All 28 normal models gain surfaces (130,797 subdivided donor triangles before
backface duplication; 31 new material draws across the archive). Native source
hashes match the verified v18 checkpoint, so no runtime rebuild was necessary.

Offline textured previews at private local/v20-textured-comparison.jpg and
v20-textured-front.jpg show actual rear caps/undersides and the normal sides for
EU rifle, EU MG, shotgun and PAC rifle. DDS is decoded from owned archives using
private tools/video-deps/imageio_ffmpeg; no game assets/video/dependencies enter
source. Preview is not native lighting, animation or headset proof. New faces
may show lower-detail donor textures or seams; the next normal playthrough is
the verification boundary. Current game remained running, so stage via the
checked next-launch selector; never overwrite the mounted weapon ZIP live.

Final v20 deployment: the game subsequently exited, and the guarded selector installed the completed archive. Its installed SHA256 matches ff05f50c9f6e9f9ddcdce1f2e9ab649fdd585ab7e9839112ae26b07de9b6d02b. The usual shortcut selects v20 assets with the unchanged v18 runtime. Private local/stereo-v20-checkpoint.json records the installation and verification; in-headset testing remains pending.

## BF2142 v21 fix checkpoint; next larger interaction pass

The user confirms v20 weapon models are fixed in-headset. New reports: EU MG
uses broken native flat ADS and a surrounding zoom/flicker; deployment laser
and native mouse disagree. User requests sight alignment instead of right-grip
ADS. They then requested a separate saved fix build followed by the larger
interaction features without intermediate headset testing because Quest/Steam
Link disconnects on headset removal. Preserve v20/v18 as the known-playable
fallback and freeze v21 before further source changes.

Native HUD root constructor EXE+0x46e020 initializes left/top (root +4/+8) as
negative half width/height (+0xc/+0x10). Pointer Move EXE+0x46dcd0 stores canvas
coordinates at root +0x38/+0x3c; the old adapter used UV*extent, losing origin.
Read finite bounded left/top with existing owner/profile guards and use
left+u*width, top+v*height in both Move and Update. Regression fixtures now
include centered 800x600 and 1600x900, arbitrary origins and corner/center hits.
The fixed panel geometry and Flash/Windows cursor paths are unchanged.

EU MG geom0/LOD0 alpha glass bounds: x +/- .01364, y .07386..09740,
z -.1329. Profile inset center(0,.08563,-.1332), half extents(.0123,.0104),
rectangular; native zoom factor .59 (owned Weapons_server tweak). It is 1x:
keep normal world pixels, draw only a lens-clipped reticle, skip scope replay.
Native factor/fine zoom validation remains, but cannot turn this reflex profile
into magnification. Existing five magnified profiles retain lens-only replay.
All six get normal first-person visual LOD on native zoom. Unknown profiles
keep native behavior. Alignment auto-ADS no longer accepts a grip override.
Default AutomaticADS=1; set 0 for legacy grip operation. Native cancellation,
local ownership, freshness and manual keyboard zoom ownership still apply.
No cached whole-arm ADS pose is re-enabled; accepted hand source is unchanged.

Both architectures build; complete Win32 suite 47/47 passed. Standalone GPU
fixtures additionally cover reflex without a scope replay. No headset run yet.

Larger requested work: body slots (back primary, holstered sidearm, chest knife,
belt utilities), controller-capacitive finger poses when practical, visible IK
and eventually replicated waving, and vehicle cockpit reconstruction. Add a
headset-free desktop simulation path for development rather than asking the user
to reconnect between builds. Native multiplayer pose replication and cockpit
art require separate evidence; do not represent local-only IK as networked or
procedural placeholders as completed vehicle interiors.


## BF2142 v22 interaction batch / desktop testing

Preserve the frozen v21 checkpoint and the user-confirmed v20 geometry with v18
runtime as separate fallbacks. User explicitly requested no intermediate
headset tests; use desktop mode and deliver one larger build.

DesktopSession is a StereoSession mode selected with `@desktop` by
`--desktop-vr`. It supplies synthetic poses into the existing shared control
block, acknowledges its own frames, skips D3D11/OpenXR transport and composites
one eye + UI into the original D3D9 window. Focus/age/owner guards stay active.
Native inventory reads are bounded and local-foot-soldier only. Equipment uses
normal number-key input, with no native inventory writes. Body props come from
a separate local owned-asset pack and never alter v20 geometry. See HANDS/PORT
for visual limitations and keyboard controls.

Touch actions are optional and enabled only by the BF2142 presenter mode
(enableGameShortcuts=false). If extended Oculus binding suggestions fail,
retry the unchanged original profile. Inactive/unavailable touch falls back to
analog trigger/squeeze. Extra shared hand flags preserve transport structure
size and BF1942 ignores them. BF1942 creates no additional touch actions.

Remote multiplayer pose synchronization and reconstructed cockpits are NOT
implemented; do not equate expressive local first-person hands with remote IK.


## v22 verification record

Both x86 and x64 development builds pass. Full Win32 CTest: 49/49. Nine
GPU fixtures pass: renderer, stereo/MSAA, magnified scope/MSAA, reflex/MSAA,
and desktop composite/MSAA. Desktop bot matches connected native cameras,
controller hands, ADS and shot mapping. Read-only inventory inspection found
and corrected unequipped items having null physical parents and native slot
4 being kit equipment / slot 7 grenades. Live body grabs selected primary,
pistol, knife and grenade. Simulator function keys are blocked from native
keyboard input so trigger/menu/capture cannot also change camera or seats,
including during simulated tracking loss. Final headset validation is pending.


## BF2142 v23: v22 headset feedback / hand and holster corrections

The user confirms v22 physical sight activation/rendering works, but their
2026-09-23 video shows poor body attachments and displaced gun grips. Keep the
v22 checkpoint (including its repaired OpenXR runtime payload) and v21/v20
fallbacks intact. Never regress the accepted sight path to fix body positioning.

HandBindingCache previously froze rightFromWeapon on the very first observed
equip frame while only the left foregrip settled. Both now settle independently
after 750 ms equip age and 250 ms stable observations, using fresh native
animation provisionally. Invalid/gapped samples restart convergence; active
supported ADS cannot settle either binding. Settled grips remain fixed through
reload/ADS. Wrist targets, shot mapping, support geometry and free-finger policy
are otherwise unchanged. This addresses a concrete capture hazard; it is not a
claim that every weapon's grip is headset-verified.

BodyInventory must NOT obtain body yaw from MakeYawOnlyUiAnchor: that UI Euler
extraction changes heading under combined yaw/pitch. Flatten the normalized
head's actual forward vector, retain heading near vertical/downward gaze, and
keep equipment upright. Compensate eye movement about a nominal 12 cm down /
8 cm rear neck pivot. Recenter resets the torso explicitly. Controller validity
is separate from anchor validity: a hidden/behind-back controller cancels grabs
but cannot place all props at the default tracking origin. Tracking return with
grip already held cannot select an item. Invalid head/menu/focus suppress props.

BodyEquipmentPlacement separates a slot's grab point from the mesh centroid.
Primary stock/knife handle/pistol top sits at the grab point with the long part
hanging down. Knife representation is capped at 30 cm; stock 62 cm ammo boxes
are uniformly fitted to a compact belt envelope, not rendered at deployment
size through the arms. Grenades stay upright and belt utility thin faces align
to the hips. Scaling applies only to holstered representations, never held game
models or bullet/optic frames. Invalid anchors do not render any props.

Residual limitations: body heading is inferred from HMD without a waist tracker;
there is still no native world/hand depth for holster props, no full torso mesh,
and no remote multiplayer IK. v23 is a private correction build requiring a
normal headset playthrough; desktop/unit checks do not prove headset feel.


v23 verification: full Win32 CTest 49/49 and all nine GPU fixtures pass on the
final client, with x64 presenter build passed. A desktop bot match exercised
native hand solving, settled both grips, sight activation and body drawing.
A bounded opt-in BF2142VR_HAND_CAPTURE path captured 16 before/after skeleton
snapshots (30 callbacks apart); first equip has a materially different wrist/
weapon relation. Native inputs are regenerated in local space before solving;
post-update world-space part matrices are normal native processing, not proof
of pose accumulation. No new world-space workaround was added. The diagnostic
is disabled unless explicitly provided a private output path. The captures and
owned asset pack stay outside public source. Headset feel remains unverified.


## BF2142 v24: motion actions, controller fingers and face clearance

The owner accepted v23 in-headset on 2026-09-23: body placement, gun hand
positions and physical sights now feel correct. Preserve that checkpoint and
its grips. The clarification for fingers requests controller-touch animation,
not changes to general wrist position or controller-free finger tracking.

MotionActions is a pure timestamped policy for exact local `knife` and
`unl_grenade_frag` names. Knife uses a rested, deliberate stroke, speed/travel
thresholds, 900 ms draw grace and one pulse per stroke. Head/hand translation
in common does not count as a strike. Frag: hold right squeeze, new trigger
edge primes (short haptic), squeeze release throws with smoothed controller
velocity. A stationary release drops it. Equipping, menus, focus/tracking loss,
recenter, jumps and stale samples reset gesture history. Body draws consume
input first. Ordinary firearms and menu clicks retain existing mappings.

NativeHands keeps the verified local soldier/inventory/fire-interface guards.
The gesture captures launch position/direction before the stock animation
finishes. Native firing still owns ammo, damage, cadence and projectile creation.
The profiled velocity getter at EXE+0x1c1ac0 (weapon interface +0x1a0, vtable
+0x1b0) obtains the speed already used by the native driver. Frag replaces only
that forward velocity component, preserving inherited soldier movement; knife
keeps native projectile speed. Expiry is 900 ms knife / 1800 ms frag to cover
hard/soft native throw delays. No template or network fields are overwritten.
Priming is an input state: native pin/throw animation runs on physical release;
there is no fuse cooking or fully manual pin animation. The native launch delay
remains. Motion controls require a real headset playthrough before acceptance.

Knife uses controller grip orientation with the authored wrist-to-item binding;
rifle/pistol aim/grip alignment and v23 settlement are unchanged. Both hands now
animate thumb/index from capacitive touch and analog trigger. The three holding
fingers remain authored; support grip keeps all four gripping fingers authored.
The free hand retains analog squeeze curls. This is estimated animation from
Quest controller inputs, not independent tracking of all five fingers.

WeaponFaceFade uses the private owned world-model pack as an approximate
collision surface for the held model. Triangle distance / inside-volume tests
choose one opacity for both eyes: clear at 7 cm, invisible by 2.5 cm or inside.
Only during proximity, replay each native eye with the existing first-person
near-clip exclusion and zero animation delta, then blend away the first-person
layer. Keep HUD, head, wrists and ballistic aim unchanged; suppress the lens
while fading. This can cost two additional renders near the face and fades the
hands with the weapon. Unknown/missing mesh data leaves normal presentation.
This is visual clearance, not physical weapon/world collision.

Settings `MotionActions=0`, `FingerPoses=0`, `WeaponFaceFade=0` independently
restore their prior behaviors. The desktop simulator adds decimal+keypad motion
(2.4 m/s) and keypad 1/3 right index/thumb touch. Manual magazine/bolt handling,
remote multiplayer IK and rebuilt cockpits remain future work.

Validation: x86 client/launcher/translator and x64 presenter built; 51/51 CTests
and eleven GPU fixtures passed, including native firing-owner/velocity fixtures,
tracking-gap input suppression and MSAA/non-MSAA near-face replay. These are
simulated/native-layout fixtures; no v24 in-game or headset acceptance claimed.
The owner's existing v23 process was left running during development.

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


The remote client adapter hooks verified EXE+0x1efc00 after native third-person
finalization. The accepted EXE+0x1ee980 first-person hook does not run for remote
players. Both callbacks pump the bounded receive path, so an observer does not
depend on a live local 1P animation. Rig writes touch only arm/finger ranges
14..44 and item attachments 64..71 of the verified 80-bone palette. One unreachable
hand falls back independently; it cannot cancel a reachable opposite hand.
Unknown topology, stale poses and wrong-thread callbacks retain native animation.
The source contains the complete server pump/AI-field and remote-hook corrections;
private live-session bridges are not runtime dependencies and must not be shipped.
Instant Action versus multiplayer feature differences were explicitly deferred by
the owner. Do not change accepted first-person grips, normal shortcuts or public
release payloads while finishing this multiplayer experiment.

Remote native blend transforms must use InverseAnimatedBone, not a transpose:
a captured near-rigid rig reproduced compounded scale/shear rejection. The exact
3x3 inverse fixes it without relaxing the existing pose guard. v31f desktop
validation now includes a visible raised-arm bot mirror and target-position
sampling through the real dedicated relay. See LOCAL_MULTIPLAYER.md for the
explicit limitations; these are simulated controllers, not a second headset.

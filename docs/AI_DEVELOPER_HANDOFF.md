# BFVR AI and Developer Handoff

## Water texture compatibility and EU sight artwork candidate (2026-10-03)

NativeExResources extends the existing managed 2D SYSTEMMEM shadow rule to
usage-zero MANAGED cube and volume textures. Preserve CPU row/slice layouts;
tag all subresources, upload to the DEFAULT-pool GPU texture after unlock, and
keep COM-private-data ownership and per-vtable original-method dispatch. Other
resource pools/usages retain their former path. NativeExWaterTextureSmoke
demonstrates the former cube pitch mismatch and verifies all faces/mips,
subresource writes, GPU samples, R5G6B5 volume normals, DXT1 cubes and ResetEx.
Do not ship this development probe in the installer.

OpticDefinition::euRifleBrackets selects only eu_ar_rifle and eu_ar_rocket,
which share the same glass. CPU fallback and GPU composition draw blue paired
brackets plus the existing red bore-zero dot instead of the captured native
scope artwork/rangefinder. Preserve aperture clipping, sight alignment,
magnification, eye selection and state restoration. Other optics retain their
native HUD/fallback rules. Existing GPU smoke compares all 96 profile/HUD/eye
cases; the native tests also verify both channel orders and range-HUD exclusion.

The code is based on the accepted beta.4-hotfix.1 source. Both builds and 76
native tests pass; no current headset acceptance or publication is claimed.
The residual Carbone Island seam also reproduced in ordinary flat D3D9.
Its water/EnvMap.dds is a single 2D scene image where the water shader expects
a cube. Near/shadowed water uses reflection sampler 1 (shadow sampler 0);
distant variants use reflection sampler 0. Do not replace a fixed sampler slot.
The private desktop comparison eliminated the line with a valid cube. A first
panoramic projection flattened reflection detail; the accepted conversion
repeats the owned scene view, stitches shared edges and box-filters all mips.
Missing authored directional views are approximate, not reconstructed.

WaterReflectionRepair.py applies only to the exact known malformed image in
the optional Carbone Island client archive. It generates locally, verifies a
deterministic output identity, preserves all other entries and stages a map
backup. Setup/update records this separate game-file mutation; the update
journal restores both map and weapon on failure or interruption. Uninstall
restores the original map archive. Missing maps, already valid cubes and unknown
reflection images are skipped. No water texture or diagnostic runtime hook is
distributed. Run TestWaterReflectionRepair and TestSetupUpdate with the setup
Python environment. Desktop owner accepted the revised reflection. Exact ZIP fresh install,
previous-release upgrade, repair and byte-exact uninstall checks pass, including
the repaired map. Main installation retains all 112 maps and the accepted
weapon archive. No new headset verification is claimed.

## Accepted beta.4 merge (2026-10-03)

Supersedes the pending-headset status in the earlier private candidate notes.
Owner reported crates with a gun working, shimmer apparently fixed, then all
interactions working after an initial PAC grip/old-weapon transition report.
No further native edits were made after that acceptance. Preserve that runtime.
ThinShellBackfaces + PlaneSeparatedBackfaces are now the installer geometry
path. Their output must match the accepted 28 per-mesh identities and complete
archive hash. Keep original surfaces and world LODs untouched. Legacy public
completed archives can be upgraded directly. Older private smooth-offset
archives require their verified stock/legacy backup; never offset them twice.

Launcher keeps a versioned copy outside the mutable runtime directory. Signed
feed schema/channel remain compatible with existing updaters. Check hashes
against the signed release, never trust only an installed version string.
Diagnostics collect launcher/setup text only, redact common personal data,
and are exported by the user. GitHub button opens a draft, never submits or
uploads. No account, game, generated-asset or microphone collection.

## Offhand crates and acknowledged selection (2026-10-03, private candidate)

SupportCrates can defer equipping a crate while a supported gun is held. The
local body-equipment GPU renderer draws its held preview; native support grip
is consumed, while gun trigger input stays native. Release caches the left pose
and velocity, then selects the crate. Wait for a fresh tracked crate fire pose
before issuing the native throw, and restore the prior gun after ammo depletion.
This deliberately pauses gun firing during deployment. It does not fire inactive
inventory objects or alter fire timers, ammo, server logic or wire protocol.
The preview is local; remote clients see existing native deployment behavior.
Right-hand grabs, focus/tracking/owner loss cancel the operation. Preserve the
one-shot empty-hand holster for grabs begun without a gun.

BodyInventory now retains unacknowledged intent for up to 900 ms, with one input
attempt per 180 ms. Never renew the gesture timestamp each frame. DirectInput
state/event paths share attempt ownership; ControllerInput re-reads the actual
native equipped slot on delivery. Once acknowledged, InputOverlay latches that
gesture complete even if a stale later sample says otherwise. This supersedes
the former single-attempt policy below; never retry active-slot selection.
Repeated XR timestamps are not tracking gaps. Grip hysteresis releases below
0.35 after pressing above 0.65, consistently in inventory and WeaponGrip.

HandBindingCache permits up to 48 cm lateral separation only for as_av/as_aa.
Owned stand-animation inspection establishes their 41.6 cm side handle; the
rifle limit remains 30 cm. Forward, distance, stable-draw and ADS checks remain.
Do not modify authored grip positions or the accepted hand solver for this fix.

Both builds, full 76-test suite, and the native Ex equipment GPU smoke pass.
Actual in-headset interactions remain unconfirmed. A private thin-shell mesh
candidate limits per-face inset against the nearest opposing surface of the
same rigid part; sub-quantization inner faces are discarded, exteriors retained.
9 synthetic geometry tests and all 28 owned-mesh preservation/bounds checks pass.
Do not promote that mesh algorithm into installer repair identities until the
headset shimmer comparison succeeds. Preserve the accepted pistol bounds fix.

## Repaired weapon bounds and PAC pistol protrusion (2026-10-03)

The private per-face candidate left LOD bounds unchanged after moving added
vertices. Owned GPU capture confirms the native signed-short position scale
uses the union of recorded mesh extents. The PAC pistol has 21 out-of-range
vertices: packing its offset magazine underside wraps negative Y upward and
reproduces the reported tall dark plate by the rear sight. Donor sight trimming
did not fix it; restore those surfaces. This is a bounds error, not damaged art.

expand_first_person_bounds includes every encoded vertex (even unused ones),
expands without shrinking existing extents, retains part counts, and leaves
world LOD geometry/bounds unchanged. Apply after the last position change.
Both smooth-normal source and private per-face candidates are corrected;
installer identities/upgrade input recognize the prior unbounded source output.
CPU packed-position reproduction is clean; owner confirmed the PAC pistol in VR.
This does not establish that the separate faint shimmer is resolved.

## Local hand roll and early holster grabs (2026-10-03, private candidate)

BodyInventory retains an intentional right squeeze for at most 200 ms before
slot contact, then consumes that intent once. This is separate from the existing
120 ms selection publication and stable SelectionTime. WeaponGrip must accept
the resulting one-shot slotGrab even without a new squeeze edge on that frame.
Never renew a selection gesture each sample or retry it by repeating slot keys:
that previously toggled fire mode and restarted native equip transitions.
Release, invalid tracking/focus and time discontinuities cancel pending intent.

PoseEmptyFingers uses a common anatomical hinge for each non-thumb finger,
mapping both segment direction and bend axis rather than unrelated shortest
arcs at every joint. A deeply curled, slightly nonplanar right finger can otherwise
twist when opened. Preserve original lengths and bone-to-segment alignment,
the thumb path, wrist/weapon transforms and pistol cup placement. Changes are
local empty/free-hand posing; this does not replace remote-player IK.
Mirrored folded-finger, early-grab/cancellation and subsequent-swap regressions
pass with the full 76-test suite and both builds. Owner accepted the empty-hand
appearance in VR; holster reliability remains unconfirmed. Retain that runtime.

## Repair-added weapon surface flicker (2026-10-03)

Status update: the smooth-normal installer correction described below was
desktop-clean but still shimmered in VR. A private candidate translating each
reverse triangle along its geometric face normal substantially improved the
headset result; residual shimmer and PAC sight completion are under evaluation.
Do not publish the current installer correction as a fully validated solution.

Live desktop comparisons isolated the EU sniper light/dark flicker to repair-
added reverse surfaces. Original plus donor exterior surfaces were clean;
separately biasing inner draws reduced but did not eliminate the artifact.
Native index-buffer capture verified the tested original/reverse ranges were
not reordered. Shader/shadow changes did not resolve that comparison.

SeparateWeaponBackfaces runs after the existing completion/interior-removal
plans. It verifies paired-vertex provenance, then moves only added inner
positions 0.0005 m along their inward normals. Exterior vertices, material and
index data, UV/normal/part attributes, transparent materials and world LODs
remain unchanged. Do not replace this with global depth/culling changes or
ship the private primitive-count diagnostic. Owner confirmed the corrected
EU sniper clean using the normal renderer with all surfaces retained.
All 28 stock repair meshes pass invariants; that is not 28 headset tests.

The accepted archive identity changes. Setup accepts the previous completed
archive as a known upgrade input and verifies all new per-weapon outputs.
Archive-changing updates keep a separate verified rollback copy, journal its
before/after identities, and preserve the original uninstall backup. Prepared
updates roll back the archive before restoring the old runtime. External file
changes fail closed with backups and journal retained. Nothing is published.

## Kit acquisition and ADS tickets (2026-10-03, private candidate)

SupportCrates keeps a pending intentional press for 250 ms and expands only
eligible crate slot radii by 0.035 m. It does not auto-grab from a long-held grip.
An interruption/reset, release or disabled left-crate setting cancels that
intent. SelectionTime still begins once at actual acquisition, not the earlier
press; changing it every frame previously restarted native selection delays.
Never relax native deployability or the existing one-shot throw/holster path.

NativeCrosshair classifies the existing signature-verified stock cull node once
per draw. Only TicketInfoConquestCullNode gains aimed suppression, on both base
HUD and optic replay. Do not suppress TicketInfo or OtherMapItems: they parent
unrelated UI. Titan health, commander nodes and similarly named custom roots
pass unchanged. No persistent node visibility/alpha or native HUD state writes.
StereoHudBegin derives this render-thread state from the current local native
optic, clears it for non-gameplay/menu draws and StereoReset, and preserves it
across nested Flash replay. Ticket nodes never count as valid scope artwork.

NativeCrosshairTests checks exact-node filtering, both replay paths, restoration,
scope-reticle preservation and unrelated nodes. SupportCratesTests covers early
grip, expiry, cancellation, radius bounds and native cooldown guards. The real
StereoSession GPU fixture asserts ticket state across optic/menu/pause/reset
transitions. These checks do not establish headset acceptance.

## Full stock sight audit and candidate (2026-10-02)

The owner requested all stock/default and unlock sights, without discovering
missing weapons manually. Read-only scans of both the owned original install
and repaired private game cover 86 GenericFireArm templates: 24 optical zoom
weapons, two iron-sight pistols and 60 with no native optical zoom. This includes
attachment objects and promotion variants; it is not a claim about custom mods.
See BF2142_SIGHT_COVERAGE.md and scripts/bf2142/AuditWeaponOptics.py. The audit
fails on missing optical definitions, wrong native factors, missing zoom HUD
selectors/roots or profiles absent from the selected install.

GunOpticDefinitions exposes the complete table to CPU/GPU/native tests. Heap
strings are exercised for long promotion names. Additional profiles preserve
exact native factors (notably Voss rifle .59 versus its rocket .484). AA HUD
roots move to the optic as a whole: static reticle pictures do not necessarily
have cull wrappers of their own. Native AA lock code is untouched.

Opaque SMG and PAC LMG lens meshes require a 1x world image, unlike transparent
EU LMG glass. OpticNeedsScene is shared by both compositors and StereoSession;
1x does not itself mean dot-only. SMG dots remain red and omit zoom artwork.
The --opaque-reflex integration fixture checks the extra view does not add an
animation-time advance or fall back to CPU transfer. Existing six accepted
optic calibrations and normal reflex behavior remain unchanged.

Both architectures, 76 CTests and six audit fixtures pass. The live-file audit
passes on both installs. GPU comparisons pass 96 cases at each of 512x512 and
2064x2208. Newly added geometry/eye relief still need headset validation; these
are automated coverage/rendering results, not a claim every gun was played.
GPU transition fixtures pass for magnified, opaque 1x and transparent reflex
paths: 151 publications and 91 time advances each; world replay counts are
243, 243 and 182 respectively. No public upload or SteamVR setting change.
Save separately from bd224c1.

## Accepted scope HUD; missing-sight follow-up (2026-10-02)

Owner accepted the bd224c1 scope-widget candidate: "that fixed it, looks great."
This supersedes the pending scope-placement/reticle check below. It does not
validate every weapon or the intermittent alternate Reset route. The accepted
private checkpoint is retained unchanged while missing sights are added.

New exact profiles: EU/PAC SMGs (1x red dot), EU/PAC anti-tank launchers (1.5x),
and Pilum (2x, shares PAC anti-tank HUD selector 82). Geometry comes from local
stock-model sight faces; no game data is shipped. SMG redDot policy deliberately
ignores native zoom artwork, keeps scene pixels and requires no third world
view. Existing optics retain their native HUD/fallback colors. CPU and GPU
apply the same policy. The default false field preserves existing profiles.
Native adapter now permits factor 0.85 only after the existing exact template,
component, ownership and native-factor checks. Weapons remain GenericFireArm;
no new executable hooks, firing, network, IK or animation paths are added.
Only the four exact SMG/AT scope roots join the cached draw filter; AA targeting
and hip-fire roots retain their normal behavior. New alignment/eye relief still
needs headset verification before release. Both builds and 76 CTests pass; GPU
optics pass 44 cases each at 512x512 and 2064x2208. The transition fixture retains
151 publications, 243 replays and 91 time advances. No SteamVR settings changed.

## Private scope-widget correction after headset test (2026-10-02)

Owner reports the reset/menu/ADS candidate generally working in the headset.
Remaining observations: native 2D ADS elements follow the floating HUD rather
than the scope, and the sniper reticle is absent. This accepts menu usability,
not every weapon, universal FPS, or the intermittent alternate-reset route.

Native HUD inspection confirms GuiIndex conditions are cached in named cull
nodes before Draw. Changing GuiIndex only inside HudHook does not re-evaluate
that cache. The stock cull draw at EXE+0x47aca0 uses cached alpha at node+0x14;
stock named cull type is vtable EXE+0x5bb4f8, name pointer at +0x18. NativeCrosshair
now signature-checks and hooks that Draw boundary. During ordinary HUD draws,
only exact stock scope-root names are suppressed. Optic replay forwards those
same native nodes with all five draw arguments unchanged. No node pointers or
cached state are rewritten, no HUD update/animation tick is introduced, and
unrelated menus, HP/ammo and vehicle nodes keep native drawing.

Replay validity now requires a recognized visible scope root with draw content;
an allocated/blank or unrecognized capture no longer automatically removes the
fallback reticle. This is draw-state evidence, not a per-pixel emptiness test.
CPU pixel isolation and GPU scope aperture projection are otherwise unchanged.

Both architectures and 76 CTests pass. NativeCrosshairTests reproduces cached
visibility independently of GuiIndex, nesting, hidden/unknown replay fallback,
unchanged draw arguments and unrelated HUD nodes. GPU integration retains 151
publications across scope/gameplay/menu transitions, 243 native replays and 91
time advances. Actual scope-art placement/reticle visibility needs a headset
retest. Saved private candidate only; no published release or SteamVR setting
changed. Previous accepted menu candidate remains available for comparison.

## Private reset-route follow-up (2026-10-02)

The first live GPU-menu run had a smooth hangar but cropped/missing buttons.
Creation requested 2084x2228/8x; transport later saw 1600x900/4x with no Reset
log. Read-only inspection found the installed Reset target in a Windows
compatibility wrapper while the device table pointed to unhooked native D3D9.
The private correction refreshes the primary device Reset connection around
GetSwapChain and at EndScene. Separate original trampolines plus a recursion
guard retain native forwarding and existing Ex legacy-state restoration.
Those refresh boundaries might still miss an earlier transition; a successful
subsequent launch does not prove the intermittent problem fully resolved.

Win32 rebuild and 76 CTests pass. Owner accepted the next desktop menu and
entered a map. That run did not exercise the new alternate route. After the
owner reconnected Steam Link, desktop closed normally and VR launched with
creation, intercepted Reset and transport all at requested 2084x2228/8x.
GPU publication is active; headset menu/login, spawn and ADS checks pending.
No SteamVR settings, shortcuts or public release changed.

## Unreleased menu GPU follow-up (2026-10-02)

Community test.8 feedback confirms sharp/smooth gameplay but severe menu/spawn
and scope slowdown. Code inspection confirms BeginGpuPair excluded active
menus, while main-menu room rendering read both GPU eyes to CPU and uploaded
all three images again. Frontend capture also had a 33 ms throttle. This is
GPU readback/compositing overhead, not evidence that native Flash or the room
itself was rendered in software. Native UI/game CPU costs may remain.

MenuRoomGpu::DrawTo now resolves each room eye directly to the transfer surfaces.
MenuOverlayGpu shares the existing laser projection and immutable pointer hit
coordinates. VrControlsMenu exposes the existing GDI artwork with a revision;
only changed artwork is uploaded, using a persistent system-memory texture and
UpdateTexture (no DISCARD allocation stream). Input routing, canvas/font sizing,
OpenXR settings, gameplay/IK and native animation clocks are unchanged.

Main/paused/spawn menus use the same three-slot GPU ownership protocol as the
world. Stage clears both eyes before optional room rendering; missing UI is
transparent. Optional backdrop failure clears both eyes again. Unsupported GPU
preparation preserves CPU compatibility and its frontend capture throttle.
GetRequest runs before paused-menu readback. GPU frontend publication is paced
by the runtime request and consumer ownership, without the extra 33 ms gate.
Private BF2142VR_MENU_TRACE still permits an explicit UI readback; normal menu
frames have none. The desktop path composites the same updated UI texture.

Validation: 76 CTests and x86/x64 builds pass. Hidden MenuGpuSmoke compares exact
room pixels and eight controls/cursor states at 640x360 and 2064x2208 (the latter
with locally generated walker assets), preserving native state/backbuffer and
ResetEx. Completed GPU work at full size measured 1.28 ms vs 4.25 ms for the old
room plus readback alone on the development PC; this is not headset FPS.
StereoIntegrationSmoke --desktop --native-ex --hardware --pure --msaa --msaa8
--menus passes gameplay -> active menu -> paused UI -> main menu -> gameplay
with 151 GPU publications and correct native time advancement. The --scope
combination and reflex/CPU compatibility fixtures also pass.

Candidate stays separate from published test.8 and the saved ADS-only checkpoint.
Next headset acceptance: repeated Sign In/back, spawn/revive/deploy close,
controls/cursor/laser alignment, hangar, scope FPS and below-gun HUD, then normal
unscoped gameplay. Do not claim those live checks passed or publish this yet.


## Published test.8; separate ADS candidate (2026-10-01)

Test.8 source/tag, installer, ZIP and signed preview feed are published. All 18
uploaded assets match local SHA256/size; an empty updater cache downloaded and
verified all 168 manifest entries. Exact ZIP cold install/repair/rollback and
standalone test.7 upgrade tests pass. Feed filename uses binary/NUL-safe Git tree
input: Windows text-mode stdin otherwise creates an unwanted trailing CR.

The ADS candidate is a separate branch. The confirmed code path previously
forced two full-eye reads, optic/base HUD reads, scope readback, CPU composition
and three CPU uploads. GpuGunOptics now captures/composites directly on D3D9 GPU
surfaces; shader work is clipped to projected glass. Magnified optics retain
the third native view and its zero animation delta. Unsupported GPU preparation
retains the CPU path; a failed draw does not publish a partial stereo frame.

NativeCrosshair now resolves ADS selector independently of crosshair alpha and
the hide-crosshair toggle. NativeMenus wraps standalone/nested Flash batches;
a thread-local replay guard keeps genuine scope HUD replay intact. This closes
code-level escape paths; do not claim the reported in-game leak is visually
fixed until tested. No game files or IK/input/shot transforms changed.

24 GPU/CPU pixel-reference cases pass at both 512x512 and 2064x2208, including
all six sight profiles, off-eye clipping, HUD alpha, rigid transforms, hostile
native state and ResetEx. Complete native Ex stereo fixtures show 60/60 GPU
frames for magnified and reflex sights, and exactly one renderer time advance
per pair. Actual FPS gain and live HUD appearance remain unverified.


## Test.8 release candidate (2026-10-01)

The accepted bounded-upload renderer is being packaged with the test.7 installer
repairs. Installed Play VR now requests --headset-resolution and native D3D9Ex
with managed staging. Diagnostics/profile/debug/menu tracing default off.
Runtime recommendations are bounded by RenderCanvasPolicy, with compact desktop
mapping retained. The accepted headset session below supersedes older failed
candidate notes. Do not change global SteamVR settings.

ADS currently forces full-eye GPU readback and CPU compositing; magnified optics
add another native scene render. Preserve this accepted renderer release while
developing that follow-up separately. No ADS improvement is claimed in test.8.


## Live bounded-upload check: Sign In succeeds (2026-10-01 22:34 EDT)

Owner requested the saved test. Verified staged hashes and launched the bounded
upload candidate in SteamVR at actual 2064x2208/8x. Read-only shared UI capture
shows logged-in Video settings, confirming this Sign In attempt succeeded.
Menu private memory held 531-607 MiB versus 3113 MiB previously. The map
subsequently loaded with GPU stereo active. Repeated unscoped batches measured
60 pairs in 661-675 ms (89-91 fresh pairs/sec). Game memory was 1424 MiB with
a 1975 MiB largest free VA block. Brief scope use is not a sustained scope test.
Owner now reports smooth gameplay and a clear image: accepted for this live
run. SteamVR compositor log confirms HMD 90 Hz at recommended 2064x2208.
The earlier 89-91 fresh pairs/sec follows that runtime cadence; later heavier
batches can dip below it. This is not evidence of sustained 120/144 FPS or
long-session stability. Game/presenter remain running; no agent input, settings,
shortcuts or release changes.
This supersedes the closed-game/pending-login statements below.


## Latest: reproduced upload allocation surge fixed; live check pending (2026-10-01)

This supersedes earlier unrun/still-open statements below. The owner ran the
Sign In diagnostic and reported another stall. Both button edges were queued,
entered the native window callback, and returned normally. Read-only GPU UI
capture showed the native "Contacting EA master server" dialog. Rendering
continued around 18-21 frames/sec. Native Flash counters stopped only after
foreground loss; do not confuse that with the initial failure. The presenter
later exited and the agent closed only the stalled game after diagnostics.

At 2064x2208/8x, game private memory was 3113 MiB; largest free VA block was
30.6 MiB despite 421 MiB total free. One thread snapshot resolved to
FrameCapture::ReadSurface in the driver; a single sample is not a deadlock.
OpenSpy TCP/TLS probe succeeded without account data; installed OpenSpy is
newer than the legacy DLL linked by the FAQ. Do not downgrade it or claim
that either credentials or a service outage caused this issue.

A desktop-only comparison at the SAME actual 2064x2208/8x source stayed at
502 MiB. It did not log in and is not proof of login behavior. A standalone
32-bit producer/consumer fixture then reproduced 2995 MiB by 60 uploads, with
no game or OpenXR involved. UpdateSubresource's driver upload allocations are
the reproduced problem, independent of input/login or the optional lobby.

BF2142 sets SharedTextureRequirements::boundedCpuUpload. PublishFrame lazily
owns one CPU-write STAGING texture per slot, uses Map(WRITE) rather than
DISCARD, honors both row pitches, and copies to the original shared textures.
Reusing a staging allocation waits for its prior copy; keyed ownership and
frame sequence protocol are unchanged. Partial failure returns every acquired
slot to key 0 and does not publish. Shutdown releases staging textures. The
default remains false for other games. GPU world publication is unchanged.

The new manual BF2142VRCpuUploadSmoke checks 120 full-size patterned frames,
padded pitch, alpha, keyed ownership, and a sustained memory-growth bound.
It held 159.2 MiB. Native Ex full-size GpuPublicationSmoke passes CPU/GPU
alternation, stale-HUD clearing, managed lifetime and reset. Both builds and
all 76 CTests pass. Private Play-VR-Bounded-Upload.ps1 and its own x86 folder
are staged with verified hashes. Earlier diagnostic binaries remain intact.

This is an allocation fix with hardware evidence, NOT confirmed resolution
of the owner's Sign In stall. Next check is real headset Sign In, memory,
menu response, then a map at the same native size/High settings/8x. Preserve
SteamVR settings. No live game is left running, no shortcut changed and no
release published. User has not yet answered whether room/laser motion also
froze; do not state that observation as fact.


## Latest: high-resolution Sign In stall; game closed (2026-10-01)

This supersedes the pending/running statements in the older entries below.
Owner accepted the fast native Ex 1600x900 headset run: menus worked, terrain
had no black strips or other observed visual bugs; textures remained blurry.
Unscoped gameplay was about 90 new pairs/sec, scoped fallback about 45-57.

The subsequent 2064x2208 runtime-source runs repeatedly stalled on Sign In.
Owner closed the last run. The final sample still advanced about 18 frames/sec;
presenter exited healthy with 1822 consumed/submitted frames, no reused frames,
and no reported rendering error. Game private memory was about 3.3 GB. This is
not proof of a graphics deadlock or of memory exhaustion. Native login/input
progress was not instrumented. The owner changed game video settings to High
before these runs, so this is no longer a resolution-only A/B. Preserve those
settings. Do not change SteamVR settings or manipulate the desktop in a live test.

A hidden D3D9Ex/8x/PUREDEVICE fixture using the actual private lobby assets did
not reproduce the failure: 1600x900 UI readback 1.13 ms and stereo lobby/readback
2.67 ms; 2064x2208 2.18 ms and 5.28 ms. Its private memory after 60 frames was
125/200 MiB. This isolates the optional lobby, not the whole game/login or XR.

New BF2142VR_MENU_TRACE=1 plus BFVR_DIAGNOSTICS=normal/deep enables diagnostic
click queue/native callback enter/return logs, native Flash begin/end counts,
controller sample acceptance, sampled UI-change counts, foreground ownership,
and private/available virtual memory every five seconds. No text/keys/account
data or images are logged. It does not change input/rendering policy and defaults
off. Both builds and all 76 CTests pass. Do not present this instrumentation as
a fix. The separate private Play-VR-SignIn-Diagnostic.ps1 is staged but UNRUN;
it uses its own x86 binary folder and leaves the earlier candidates intact.

No confirmed root cause, high-resolution acceptance, shortcut update or public
release. Next real reproduction needs the diagnostic log and, if callbacks stop
returning, a local thread-stack capture before closing. Ask whether the laser/view
kept moving or the entire view froze; that question is currently unanswered.
No game was launched during this investigation.


## Latest headset comparison: native Ex at the same 1600x900 source

After a fresh SteamVR session, the native Ex comparison reached live gameplay.
The ordinary-renderer control was accepted for working menus but still blurry.
It measured roughly 47-61 new frames/sec, with a scoped batch about 42/sec.
The subsequent native Ex 1600x900/8x run logged 60 unscoped world pairs per
~660 ms (about 90/sec) with eye/HUD readback eliminated. Scope fallback measured
about 45-57/sec. This is actual headset-session evidence; user confirmation of
menu stability/terrain remains pending. Memory was around 1.6 GB in that run.
This does not establish the cause of the earlier freeze or accept high resolution.

Private Play-VR-Fast-HeadsetSize.ps1 is prepared for a resolution-only comparison,
with runtime query, FXAA off, the same native Ex/staging settings, and normal
logging. It never modifies SteamVR settings. Do not interrupt the current match
until the owner finishes the pending comparison feedback. Normal player shortcut
and public assets are unchanged. Keep the earlier failed-run record below.

## Headset attempt: menu freeze, stable-renderer control now running

The first native Ex headset attempt was rejected: the owner pressed Login,
reported a yellow pointer and a frozen view. Producer/consumer/request counters
all remained at 2519 in two live samples; neither side reported an error in the
shared block. The x86 working set reached roughly 3 GB. Diagnostic logging was
off for that run, so there is no established root cause. Steam Link recorded
stream resets/timeouts, including before this launch; this alone does not assign
blame. Do not call the fast path headset-accepted or publish it.

The runtime initially recommended 920x984 at a global 20% resolution setting.
The agent changed that global setting to 100% for the first attempt (2064x2208),
then restored the previous 20% after the owner objected. Do not change global
SteamVR settings again as part of a mod test. Keep settings changes explicit and
isolated from renderer comparisons. Runtime recommendations can remain cached.

Windows retained an exited game object after the failed run. The launcher now
ignores only instances whose exit code confirms termination; live or inaccessible
instances still block. For this confirmed-exited case only, the native supported
+multi 1 switch avoids BF2142's own stale-instance rejection (exit code 42).
The subsequent real launch succeeded; all 75 CTests pass, though no dedicated
stale-process automated fixture has been added yet.

The control session uses ordinary D3D9, the menu-compatible 1600x900 source,
latest FXAA-off presenter, unchanged native gameplay/IK settings, and opt-in
normal diagnostics. Live shared counters advanced 357->412 and presentation
642->698 over one second, with fresh focused controller tracking logged. This
proves a working control launch, not accepted clarity or fast-renderer FPS.
Await the owner's visual/menu feedback. No public release/normal shortcut update.

## Latest private follow-up: stereo ground regression cleared on desktop

After the resource-original dispatch fix below, the owner confirmed Suez Canal
without black strips at 1600x900 and again at 2528x2704, both 8x MSAA. Logs
confirmed stereo remained active and shared GPU frames avoided eye/HUD readback;
approximately 100 desktop pairs/sec is not a headset measurement. Keep
`BF2142VR_GPU_TRANSFER=dx9ex` plus `BF2142VR_EX_MANAGED_UPLOAD=1` together for
this candidate; their individual contributions are not yet isolated. High-canvas
CreateDevice/reset/capture dimensions now agreed in the successful run. Earlier
flat fallback and rejection records below describe earlier experiments.

Community A/B reports identify FXAA as the fuzzy-text/detail cause independently
of bloom. BF2142 now uses a separate persisted `bf2142_fxaa_enabled` key, default
false, selected from the producer flag; other games keep `fxaa_enabled`. Native
MSAA and bloom are unchanged. Use an explicit private presenter config path for
the headset test. Do not promote experimental backend/default source sizes to
normal launches until the headset check. No public release or shortcut update.

## Continuing native Ex investigation: do not stop at rollback

The owner explicitly asked to continue fixing the fast path after the ordinary
D3D9 rollback. Reset semantics and PUREDEVICE were tested: native Ex retains
blend/scissor/depth-bias/texture state across both Reset and ResetEx, unlike
ordinary D3D9. NativeExReset captures initial state before game/overlay setup,
restores only at legacy Reset, and sets viewport/scissor to the NEW backbuffer.
Never apply this block between eyes. Preserve original native Ex creation flags;
the rejected On12 path still strips PUREDEVICE. These corrections passed hidden
fixtures and 75 CTests, but the owner confirmed black strips still present.

Preserving buffer Usage (DEFAULT VB/IB do not require DYNAMIC for Lock) also did
not remove the reported strips. Expanded diagnostics now cover surface, cube,
volume-texture and NOOVERWRITE locks. All remain opt-in. That candidate stopped
stereo at `Desktop composite failed at upload lock hr=0x8876086C` during menus,
then continued flat; do not describe its in-map performance as stereo.

Private hidden layout probe: e.g. 16x16 A8R8G8B8 managed/system-memory rows are
64 bytes while Ex DEFAULT+DYNAMIC rows are 128 bytes on this adapter; small A8,
L8, R5G6B5 and R32F also differ. `BF2142VR_EX_MANAGED_UPLOAD=1` now keeps an
owned SYSTEMMEM shadow for managed 2D textures with Usage==0 and uses explicit
UpdateTexture after unlock. COM private-data references own texture/mip shadows
without pointer maps or GPU-parent cycles. Other texture usage/cube/volume paths
retain existing translation. This is a comparison, not complete managed-pool
emulation: uploads currently mark the full chain dirty, including read-only
unlocks, and native SetLOD/PreLoad behavior is not emulated.

The owner confirmed "suez canal baseline, stripes are gone" with that upload
option. The same run had a menu upload-lock error and was flat by that point.
Record this limited positive result; repeat it with functioning stereo before
accepting. NativeExResources also incorrectly reused one original trampoline
for different runtime implementations of a COM method. Resource hooks now look
up the original by the object's actual vtable entry. A mixed-resource fixture
includes a pre-existing dynamic menu texture after managed asset registrations;
the corrected build is undergoing native menu/world verification.

ResetEx slot 132 and additional-swapchain creation are now traced for the separate
high-canvas startup issue. A direct Ex reset retains Ex state semantics; legacy
Reset owns compatibility once through a reentrancy guard. No high-canvas or
headset acceptance is claimed for these changes. Normal shortcut/public payload
remain unchanged; keep diagnostics and both experimental options off in them.


## Native D3D9Ex candidate: readable menus, ground rendering blocker (unreleased)

The old `BF2142VR_GPU_TRANSFER=1` On12 experiment below remains rejected. A
separate exact `dx9ex` value now creates the system D3D9Ex factory/device and
uses legacy D3D9 shared render targets opened by the matching D3D11 adapter.
Do not close these non-NT sharing handles. A D3D9 EVENT query completes writes
before D3D11 reads, and the producer's D3D11 query completes copies before reuse.
ResetEx releases our transient resources and retains native managed substitutes.

NativeExResources translates managed creations on the selected device only to
DEFAULT+DYNAMIC, preserving reported pool/usage through resource private tags.
The approach follows the BF1942 translator's managed compatibility policy; it
is not complete generic managed-resource emulation. Font alpha, mip/subrect,
compressed textures, buffers, cube/volume textures and ResetEx fixtures pass.
Full-size shared publication with an independent receiver and scope/reflex CPU
fallback fixtures also pass. Win32 75/75 CTests and x64 presenter builds passed.

Actual BF2142: user confirmed map labels/menus work. Desktop world frames at
2528x2704/8x MSAA reached about 100 pairs/sec with eye/HUD readbacks removed.
No native headset result or 144 FPS result exists. User subsequently reported
solid black strips across the ground which change with view direction. This
is a release blocker, not an accepted visual result. Do not replace the normal
shortcut or publish this candidate. Keep current sessions undisturbed while
preparing the isolated diagnostic build.

Further native checks: black strips persisted at 1600x900 and with the process
event selecting CPU transfer on the same Ex device. Returning to ordinary D3D9
at 1600x900 removed them; the user confirmed the improvement. This narrows the
regression to backend/device/resource differences, not GPU publication alone.
It does not establish shadows as the cause. The native dynamic-shadow flag and
the active Video.con were already disabled. Console shadow-property guesses
were rejected; a private guarded flag comparison made no write because the
existing flag was zero. Capability/texture-format checks matched across D3D9
and Ex on the tested adapter. No managed lock failure/discard/nonzero-LOD warning
was observed in the bounded diagnostic log. NOOVERWRITE and surface-level locks
were not instrumented; these checks do not prove full resource compatibility.

A diagnostic high-canvas restart also regressed menu sizing: CreateDevice logged
2528x2704, then the capture saw 1600x900 without an intercepted Reset. The menu
was enlarged/cropped. Resizing the outer window did not fix it; restarting with
the established --render-size 1600x900 restored all menu buttons. Trace native
reset/swapchain ownership before another high-canvas run. Do not silently claim
the larger requested dimensions always reach the actual source.

Current playable comparison uses ordinary D3D9 and 1600x900. Keep it running
when the user is playing. No normal shortcut, public package, server or gameplay
setting was changed. This is a diagnostic rollback, not a fast-path fix.

For a same-map comparison only, `BF2142VR_GPU_DEBUG=1` enables bounded managed
lock/LOD logging and the process-local manual-reset event
`Local\BF2142VR.CpuTransfer.<game-pid>`. Setting it selects existing CPU frame
transfer on the same renderer; resetting returns to GPU transfer. This adds no
player binding and is inactive without explicit diagnostics. Compare the ground
artifact before changing resources, shadows or camera logic.


## GPU transport integration: blocked native compatibility (unreleased)

Explicit `BF2142VR_GPU_TRANSFER=1` now selects the system D3D9On12 factory and
GPU snapshot/export/publication path. It is OFF by default. The accepted normal
launcher and release payload remain unchanged. Do not enable it for players.

Hidden hardware fixtures pass including 2528x2704, 8x MSAA, independent D3D11
receiver pixel checks, CPU/GPU publication transitions and device reset. Actual
BF2142 fails EyeRestore's DrawPrimitiveUP with E_FAIL after login, stopping the
desktop stereo path. Dynamic map names also become white rectangles. An A/B at
the identical canvas and AA with ordinary D3D9 rendered those names correctly.
The experimental backend, rather than the map files, is implicated; root cause
is unresolved. Removing PUREDEVICE did not fix either symptom. Native ValidateDevice
returns S_OK; no secondary render targets are bound. Matching hardware vertex
processing and disabled MSAA render state in the hidden fixture still passes.

`BF2142VR_GPU_DEBUG=1` enables bounded failure-state logging only. No expensive
diagnostics, backend selection, or source-size change is enabled in normal play.
The last diagnostic game was closed normally. No headset acceptance or native
GPU-path FPS result exists. See HEADSET_RENDER_CANVAS.md for ownership details.
Next: resolve native draw/font compatibility before a headset test. Do not
claim the synthetic 1.6-1.8 ms transfer measurement is BF2142 frame time.

## Opt-in headset canvas experiment (unreleased)

The first high-resolution desktop playtest was rejected for severe slowness.
FramePixels optimizations now pass scalar equivalence and hidden scope/reflex
GPU fixtures. Isolated benchmarks improved, but do not call gameplay fixed.
RenderProfile is explicitly opt-in; keep defaults off. See the performance
section in HEADSET_RENDER_CANVAS.md. Accepted player binaries remain untouched.

See [HEADSET_RENDER_CANVAS.md](HEADSET_RENDER_CANVAS.md). Normal launches retain
the accepted sizing; do not enable this automatically in installers yet.
Query the matching x64 presenter before creating the x86 game. NativeRenderCanvas
profiles BF2142's main WndProc, forwards logical WM_SIZE, maps client mouse
coordinates and fixes the desktop preview independently. Both CreateDevice and
Reset receive explicit source dimensions before saving AA fallback parameters.
Never copy BF2 executable offsets or alter the native simulation timestep.

The optional WidescreenUiCanvas producer flag changes UI layout, not shared
structure sizes or pose/network protocols. Pair candidate client and presenter.
MenuPointer and the presenter must use the same 16:9 content rectangle. Source
dimensions remain real pixel dimensions for sampling; eye FOV still comes from XR.
Desktop simulation uses native mouse coordinates without ray-to-cursor feedback;
F9 still exercises synthetic menu button edges. Actual headset input keeps its ray.

Both builds and 75 CTests pass. Hardware checks cover actual D3D9 canvas/viewport
and reset, plus real D3D11 menu aspect and transparent padding. The isolated game
confirmed a 2528x2704 backbuffer/capture with a 1600x900 preview and 8x MSAA.
These are source-sizing checks, not proof of headset clarity, FPS, successful
runtime preset changes or complete menu compatibility. Keep this opt-in until
the connected-headset checklist has passed. No release assets were changed.


## Test.7 repair plans

WeaponRepairWorker now replays bounded decision-only programs compiled by
BuildRepairPlans from the exact stock hashes. Geometry is always reconstructed
from owned inputs; the 28 expected output hashes and complete archive hash are
unchanged. Both search and replay paths remain regression checked. Never bypass
source/output checks, silently fall back to expensive searches, or ship meshes.
This reduces the workload where native worker crashes were reported; it does
not establish their root cause. No gameplay binaries changed.

## Test.6 installer / updater invariants

The setup interpreter is pinned to Python 3.13.15; update matching licenses with
future pins. CompleteWeaponSurfaces queries traverse an explicit stack.
WeaponRepairWorker isolates each repair, validates input/output hashes,
publishes verified cache entries and retries a failed worker once. The complete
archive must still match COMPLETE. Never skip a model or publish generated assets.
The report showed varying native exception codes during pure-Python repair;
it does not prove one specific interpreter defect or hardware fault.

The standalone UI is src/installer. Its public key verifies the preview-channel
feed. File hashes/sizes and HTTPS bounds are enforced before execution. Cached
and installed files are rehashed; only changed files download. SetupAssets apply
stages a complete replacement before moving the previous runtime aside. First
stock backups and INI settings retain their ownership. A game-specific mutex
excludes concurrent setup; a journal recovers interrupted swaps. Do not write
live game modules or terminate a match. Gameplay DLL/EXEs are unchanged from
test.5. See INSTALLER_UPDATER.md.



## Beta.4-test.5 native draw timing

Do not return immediately from RenderStereo just because GetRequest cannot yet
acquire the consumer texture fence or next pose. That creates outer game frames
without the corresponding native animation evaluation. Accumulating renderer
delta (RenderTimeBudget) fixes effects but does not repair all draw/readiness
transitions. AwaitRenderRequest retries inside the same engine frame, using the
presenter event and a 100 ms retry budget. An individual existing request wait
may take another 12 ms; this is not an unbounded wait. Failure/disconnect still
retains the stereo scene. Never bypass native ammunition, draw or reload timers.

Private desktop reproduction: a 90 ms consumer interval extended EU handgun and
rifle selection-to-shot delays to about 3.4 seconds. Waiting in the same engine
frame restored roughly 0.9â€“1.0 seconds including the 450 ms input/settle phase.
This used ordinary game ammo counters and a private consumer-delay probe, not
weapon-state writes. The probe is not part of the release. Tests cover consumer
ownership/pose delays, immediate readiness, timeout, shutdown and recovery.
Headset acceptance remains pending; the IK solver and wire protocols are unchanged.

## Beta.4-test.4 equipment input hotfix

SupportCrates no longer owns a 1.2-second cleanup interval after completion.
It emits one holster result; right-hand body grabs cancel any pending crate
transaction. WeaponGrip::ResolveSupport determines the effective held state
before the sole SetNativeWeaponHeld call per XR update. Do not publish a
transient right-hand-only state before applying a left-crate override: that
repeatedly rearms gripTransition and suppresses native fire.

ControllerCommand equipment selections carry local owner, gesture time and
item. BodyInventory/SupportCrates keep the gesture identity while awaiting
equip. ControllerInput checks the live signature-backed inventory again at
consumption; already-selected or invalid-owner requests become no-ops.
InputOverlay shares selection ownership between state and buffered APIs,
keeps peek non-consuming and defers release/press edges if the buffer is full.
Do not convert these gestures back to held number keys or one pulse per XR
publication. Physical keyboard selections are unaffected. This is process-local
input state, not a shared IPC/network protocol change.

Regression coverage exercises immediate post-throw draws at 72/90/144/240 Hz,
empty-hand retention, right-grab cancellation, both input API orderings, stale
selection requests, repeated publications, peeks and full buffers. Headset
acceptance of this hotfix remains pending.

## Beta.4-test.3 interaction/timing hotfix

RenderTimeBudget accumulates skipped native renderer delta (seconds) and consumes
it once in the first eye; other eyes/scope replays get zero. Carry is bounded to
250ms and reset on renderer change/device reset/flat fallback. Never change
simulation/network time to fix renderer particle speed.

Body equipment uses a separate matching MSAA depth surface, full target viewport
and explicit sample mask; restore native depth and state afterward. The engine
owns its render cache: scope HUD replay runs inside the existing native scene,
after the main HUD draw, without externally restoring a stale native state block.
NativeUiCapture swaps an auxiliary alpha target while preserving the main UI.
Scoped signature-checked GuiIndex suppression removes known ADS widget roots;
the native HUD-only replay restores the weapon root and isolates differences
inside the central UI crop. CompositeGunOptic clips those pixels to the sight.
No stock HUD assets are shipped. Unknown/missing capture retains the old reticle.

SupportCrates queues early/low-speed releases, waits for native rounds, and
explicitly holsters after completion. WeaponGrip briefly suppresses native
post-throw auto-selection; deliberate body grabs still draw immediately.
Startup movie removal is an installer mutation with a null installed hash,
verified backup and exact allowlist. Uninstall restores originals and refuses
later replacements or corrupt backups before changing any file.

Both builds, 73 CTests and hidden GPU checks pass. Final headset acceptance is
pending; faint HUD outline is unconfirmed. IK/network protocols are unchanged.


## Beta.4-test.2 native menu correction

The owner playtest rejected test.1 automatic source sizing: the launcher passed
3072x1728 and the native menu showed stretched text/unusable layout. Remove the
launcher preflight application of runtime dimensions. Default native source is
1600x900 again; runtime output swapchains keep their own recommended dimensions.
The optional x64 query utility remains available for development only. Do not
restore automatic enlargement without actual native menu/layout acceptance.
Wrist/equipment, FPS request, VSync changes and IK are otherwise unchanged.
The native soldier-selection menu is readable again at 1600x900. All 73 native
tests pass. The test.2 headset session is running; user acceptance remains
pending. A widescreen aspect ratio alone does not validate native Flash layout.
The following test.1 section is historical; its auto-sizing was withdrawn.

## Beta.4-test.1 experimental wrist / rendering candidate

WristMenu is a gaze-revealed, right-ray-selected native Enter shortcut; preserve
its release-to-arm and fire-consumption guards. Full deployment uses the existing
MenuPointer anchor and absolute native HUD coordinates. No protocol, IK, aim or
body collision changes. Commander right-click/control shortcuts are not done.

BodyEquipmentGpu draws at StereoHudBegin, inside the ordinary eye scene and
before UI target isolation. It restores a full device state block, never changes
RT/depth surfaces, and shares native MSAA/resolve. Do not also composite the old
640px CPU raster. ResetGpu releases device textures before reset. Exporter uses
nearer 3P LOD/256px texture; 128px format-1 packs remain accepted. GPU checks use
owned private packs, never repository assets.

The launcher runs the same x64 presenter identity for an instance-only OpenXR
size query before the native game/window is created. Bound widescreen dimensions
and keep native window/backbuffer/Flash canvas matched. Do not resize only one
of them. Runtime source recommendation is capped at 3072x1728, overrideable by
--render-size; query failure retains 1600x900. Higher resolution costs more work.
The source is not necessarily the runtime's exact aspect or full supersampling.

NativeFramePacing verifies BF2142 console lockFps getter/setter and registration;
it calls the native setter (including its restriction), never force-writes the
limiter or patches multiplayer checks. Request 0 only for fresh, focused actual
headset gameplay. Restore the saved cap on loss/menu/reset only if still zero;
leave console changes alone. Desktop simulation and observers do not use it.
The actual headset D3D swapchain uses immediate presentation with native fallback.

Both builds and 73 CTests pass. The final equipment/wrist GPU check passes
after earlier D3DERR_DEVICELOST device-creation failures cleared; MSAA, stereo,
state/target preservation and reset are verified. Headset/runtime sizing,
frame pacing and wrist use are not accepted in-game yet. The owner authorized
a separate experimental beta.4-test.1 prerelease without another headset gate;
beta.3 remains the fallback. Tester reports/hardware/screenshots remain private.
The exact test ZIP passed fresh stock repair/install, repeat setup, installed
hashes and launcher inspection, equipment GPU checks, rollback tamper guards,
normal/repeated uninstall and byte-for-byte restoration. The pre-repaired
archive installation path also passed. Frozen-worker failures retain logs.


## Beta.3 ladder/input hotfix

Native ladder entry checks native pitch even when the headset uses the level comfort camera. A local reproduction showed +81.5 degrees hidden downward look; correcting it enabled attachment and ~5m ascent. The owner also confirmed the permanent desktop build. NativeLookPitch supplies HMD pitch with a native-only 5-degree upward bias to pass the stock bottom-entry gate at level view. NativeComfort writes axis 5 once per generated action batch and clears other pitch values in that batch. The existing stock input adapter also carries snap yaw (axis 4), replacing the custom snap event. Signature checks cover input layout, native axis factors, weapon look scale and signed-short /100 codec. Fresh focused on-foot local VR samples only; menus, stale/lost tracking, death, vehicles and mounted traversal retain native behavior. No IK solver, rendered comfort frame or network protocol change. Tests cover batch wrap, yaw/pitch coexistence, untouched unrelated bytes, owner/focus/death/mount guards and codec convergence. Both builds and 71 tests pass. Headset and remote-server ladder acceptance is pending; release authorized by owner after the desktop climb check. Exact staged install/check/reinstall/rollback checks are recorded in release CHECKS.txt.


## Quality hotfix invariants

Keep native window, D3D9 backbuffer and Flash canvas at the same widescreen size.
NativeAntialiasing only changes supported automatic-depth discard swapchains;
creation/reset failures retry original parameters. Unknown paths stay native.
WorldMSAASamples defaults to 8 (4/2 or 0 for native behavior). Flat observers
keep their native AA. Do not restore the square-source/compact-mirror experiment.
See BF2142_RENDER_QUALITY.md and BF2142_FEEDBACK.md.


## Current BF2142 beta invariants (0.2.0-beta.1)

Current status supersedes older private notes: see `docs/ik/README.md`, `docs/BF2_POST_ALPHA_UPDATES.md`, `docs/FLAT_ADDON.md` and `docs/BF2142_BETA_READINESS.md`. Owner authorized release without another headset test; latest visual/haptic fixes are not claimed accepted. Proximity voice with vrtester is user-confirmed.

Shared client/presenter IPC **26**, pose v4/616 bytes, voice v1. Match x86/x64 packages. Independent per-hand equipment counters preserve menu/fist feedback, with guarded reminder pulses and left crate hover.

Relay arm solves can rebase validated detailed collar/arm chains into the live chest while retaining wrist-local finger/item offsets. This isolates observer arms from native recoil. Local first-person arms and existing v35g torso neutral calibration stay unchanged. Collision samples arm segments and rotates on the elbow circle; cosmetic only. Never feed filtered presentation into aim/hitboxes.

Release staging scrubs CodeView paths and verified Opus source roots in non-executable .rdata; executable sections stay byte-identical. No game/generated assets or private fixtures are published. Beta upgrades require uninstalling and preserving/moving the earlier installer folder.


## Private v35g neutral bone-frame correction (2026-09-25)

Owner's 22-22-20 recording still shows the belly issue with v35f; do not describe
v35f's waist correction as visually accepted. Reviewed 48 one-second samples
from 18 through 65 seconds of the 75.383-second file. Empty hands and rifle
both show the posture. Live observer had no cached tracked rig during the read;
no live neutral-pose validation is claimed.

Owned 3p_setup.ske plus third-person skinned-mesh inverse binds establish the
cause: the neutral bone frames have nonzero pitch. Stored SKE quaternions are
INVERSE rotations (confirmed using the BF2 Blender importer and mesh products).
The US neutral skeleton times mesh inverse binds is near identity; PAC meshes
have a small positional variation but share these orientation calibrations.
The first geometry in each mesh is first-person; use geometry 1 for 3P checks.
Native metadata at skeleton+8 is still NOT bind rotations.

SolveRemoteTorso now keeps neutral pitch calibration at joints 11,12,13,45
separate from anatomical heading/lean/look. Remove the waist bind rotation
before interpolating into the upper spine, then add each destination's bind
rotation after interpolation. A neutral model is not a set of identity bone
axes. Existing joint-pivot propagation, write/restore range 11..74, pelvis/legs
0..10 and hip gear 75..79 remain unchanged. Exact identity/topology/write guards,
accepted torso yaw, head orientation, palm/item targets, filtering, collision,
haptics and local gameplay are unchanged. Shared v25; network v4 / 616 bytes.

New independent rounded-bind regression catches the old false-upright result;
optional private authored-bind input checks actual neutrality. Both builds and
71/71 CTests pass; all seven prior captured rigs replay successfully. Offline
skinning of the owned US mesh: old neutral correction deviates up to 4.8935 cm,
new neutral solve stays within 0.001 mm of the authored result. This is numerical
and offline geometry evidence, not visual acceptance in a running headset.
Captures, decoded bones/meshes and review scripts stay private.

Saved v35g-remote-neutral / v35g-cloud-observer; existing main shortcut selects
the candidate, observer auto-matches and Previous Build restores v35f. Older
accepted v35e/v35d remain available. Existing sessions/OBS left running. No
public release, friend package, server binary deployment or service changes.

Separate owner request to restore bots: current map was Highway Tampa gpm_sl 64
with 0 bots, because the requested all-map rotation includes non-AI modes.
Changed through authenticated map control to Suez gpm_coop 16; verified 62 bots
and both accounts. No process or instance restart. All-map rotation retained.

File-format reference (no code imported into production):
https://github.com/marekzajac97/bf2-blender/blob/main/io_scene_bf2/core/bf2/bf2_skeleton.py
https://github.com/marekzajac97/bf2-blender/blob/main/io_scene_bf2/core/bf2/bf2_mesh/bf2_skinnedmesh.py


## Private v35f lowest-spine correction (2026-09-25)

Owner confirms v35e side-to-side following works. Screenshot plus recording
BF2142-VR-2026-09-25-21-59-32.mp4 shows a protruding/arched midsection, possibly
more obvious on the recon/sniper kit. Requested review begins around 90 s.
Reviewed 51 frame samples at 0.5 s intervals from 90 through 115 s of the
142.316 s file. The silhouette is present with empty hands and with the rifle;
no class-specific mesh diagnosis or continuous playback is claimed.

Previous SolveRemoteTorso began at spine3/bone 12, leaving spine2/bone 11
native. An aiming arch at 11 can therefore remain under a corrected chest.
v35f rotates 11 at its unchanged hip attachment toward a standing waist frame,
retaining that joint's native yaw and half the bounded physical chest lean.
Standing weight is smoothstep(root up.y, .92, .98), so strongly tilted native
hips retain the old lower-spine/crouch behavior. Spine3 blends from corrected
out[11], never the native aiming bend. TorsoHeading, head rotation, arm targets,
weapon alignment, existing smoothing, collision and haptics are unchanged.

IMPORTANT: native restore/write masks now cover 11..74, not 12..74. The pelvis
and legs 0..10 and hip gear 75..79 remain native. Exact previous-write matching,
skeleton/actor identity and full verified topology guards are unchanged.
Tests reproduce +/- standing lower-back arch and assert posture recovery,
parent-local attachment preservation, unchanged pelvis/legs, crouch fallback,
continuous blend and bone-11 write/restore through skipped native animation.
All 71 CTests, both builds, five prior and two newly read private rigs pass.

Live reads of the still-running observer were read-only. The local rig had
zeroed matrices and the previous player rig was prone/ragdoll; neither was
used as proof of the reported standing appearance. Two other populated rigs
were used only as additional anonymous geometry replays. Native bone metadata
at +8 is NOT a rest-pose quaternion; do not treat it as such. Game assets and
captures stay private. No class-specific texture or model changes.

Saved candidates: v35f-remote-waist and v35f-cloud-observer. Next launch selects
v35f and the observer auto-matches. Previous Build selects v35e; v35d remains
available with -Build v35d. Current games/OBS and cloud services were untouched.
Protocol stays network v4 / 616 bytes and shared v25. Visual confirmation of
the waist correction remains pending; no release or friend package upload.


## Private v35e chest-facing refinement (2026-09-25)

The owner accepts v35d's improved IK/body appearance. New recording:
BF2142-VR-2026-09-25-21-40-48.mp4; start at 35 seconds as requested. Review
sampled 94 frames at one-second intervals, 35 through 128 seconds (128.416 s
file), not continuous playback. Chest retains a bladed stance as the arms
reach across it, visible particularly at 57-62 s; no return of the earlier
chest/head bunching was observed in those samples. Preserve this baseline.

TorsoHeading now retains less native combat-stance yaw, follows the filtered
HMD heading at 75%, and admits at most about 12 degrees of additional turn
from both palms reaching ahead at torso/head height. Smooth geometric weights
exclude one-handed waves, lowered/overhead hands and behind-body holster grabs.
Palm orientation and item type do not infer torso facing. Total chest/hip yaw
is bounded at 0.78 rad. Near-vertical HMD forward loses confidence rather than
flipping the chest 180 degrees. No new state or packet fields: existing 40 ms
presentation filtering applies to the inputs; v35d connected spine propagation
and native hips/legs are unchanged. Local gameplay/aim/IK/haptics unchanged.

New tests cover stance bias, modest symmetric reach, palm-rotation independence,
single-hand/back/overhead exclusions, absent tracking, turn limit and vertical
look. Full x86/x64 builds, 71 CTests and all five private rig replays pass.
v35e-remote-facing / v35e-cloud-observer are private candidates. Main shortcut
selects v35e, observer auto-matches; Previous Build selects accepted v35d.
No cloud service, public release, friend package, account or OBS changes.
Visual acceptance of the new facing adjustment remains pending.

Owner also asked whether equip haptics disappeared after fist-bump work. Source
still sends the original right-hand holster-entry hover pulse (12%, 15 ms),
with unchanged routing and enabled configs in v35b/c/d. Separate fist counters
do not remove it. Runtime cause unconfirmed; owner suspects battery and elected
to continue IK testing. Do not claim that haptic report fixed or confirmed.


## Private v35d torso correction (2026-09-25)

The owner accepts v35c smoothing but reports chest/shoulder distortion. The
new dual-view video was sampled at one-second intervals from 20 through 35
seconds, explicitly identified by the owner as clean tracking. Raised hands
are smooth; the chest armor rises toward the head with stock aiming posture.
Do not undo the accepted pose filter, wrist/item alignment or elbow continuity.

SolveRemoteTorso previously changed only chest yaw and independently moved
lower/upper spine weights about one hip pivot. That retained weapon-driven
pitch and failed to preserve parent-local joint attachment offsets. v35d
builds a heading/limited-lean chest frame, eases spine3 from native spine2,
and bends the neck modestly toward the HMD. Each adjusted joint rotates at its
own attachment and carries its complete subtree. Neck counter-animation no
longer folds the head into the armor. Hips/legs, prone fallback, collision,
tracked head orientation and native restore-before-finalize remain unchanged.

New regressions assert attachment offsets, upright chest limits, unchanged
hips in crouch, and identical chest/shoulder/head placement with and without
synthetic weapon-pitch/counter-neck animation. The same checks pass on all
five private captured game rigs. Full x86/x64 builds and all 71 CTests pass.
This is math/build evidence; the revised body still needs visual acceptance.

Private checkpoints: v35d-remote-body and v35d-cloud-observer. Next main launch
selects v35d; observer auto-selects the matching build from the primary DLL.
The Previous Build shortcut selects v35c; explicit v35b remains available.
Current games and the accepted OBS dual-window setup were left running.
Network remains v4 / 616 bytes, local shared memory v25, cloud unchanged.
Nothing published and no friend package replaced.

## Private v35c observer refinement (2026-09-25)

Owner supplied a 141.952-second dual-view recording and accepted the v35b head,
knife and weapon visibility improvements. Review sampled 68 frames at two-second
intervals, excluding 70-75 seconds (explicit Steam Link dropout). Remaining
complaint was stiff/unnatural body motion. Owner also requested collision
constraints and haptic fist bumps. The candidate is not visually accepted yet.

RemotePresentation filters presentation poses with a 40 ms time constant and
quaternion interpolation, leaving raw FreshPose/server gameplay untouched.
Reset on identity/session, discontinuity, invalid input, weapon/held/validity
changes and a new snap serial. Item pose is reconstructed relative to the
filtered holding palm. Do not independently smooth an item away from its hand.
RemoteArmContinuity limits elbow intent changes; RemoteCollision uses bounded
anatomical torso capsules and empty-hand separation, retaining analytic arm
lengths. These are cosmetic self-collision constraints, not world collision or
physical hitbox changes; deliberately contacting weapon grips are exempt.

Torso following requires the full verified on-foot rig. Hips/legs 0..11 and
hip attachments 75..79 remain native. 3p_setup confirms gear parents 72->13,
73/74->12; native torso gear follows those bones, never the weapon. Prone rigs
retain native torso animation. The restoration cache now tracks every changed
bone, including torso/head/gear, and restores only exact previous mod writes
before the native finalize callback. Preserve this invariant during LOD work.

FistBump uses fresh authenticated Relay poses in authoritative body coordinates.
Both hands must be closed and empty; supporting a weapon is excluded. Separate,
approach and contact produce one local hand pulse, with hysteresis, cooldown,
stale/session/discontinuity guards and a 100 ms pending-event expiry. Gameplay
focus/menu/controller gates discard haptics when inactive. Flat observers and
bot mirrors cannot generate tracked fist bumps. Both VR players need the new
client/presenter for reciprocal feedback. No new server event or packet field.

Network packet remains v4 / 616 bytes, bridge compatibility v35-community.
Local client/presenter shared memory is now version 25 with separate left/right
fist counters; deploy the matching x86 DLL and x64 presenter together.

Private checkpoints: local/multiplayer-lab/checkpoints/v35c-remote-ik and
v35c-cloud-observer. Main Play-Cloud defaults to v35c. The observer launcher
selects a matching v35b/v35c runtime from the running primary DLL hash. The
Previous Build desktop shortcut now selects v35b; its observer auto-follows.
Saved account/profile and OBS role titles are preserved. No game was started
for this candidate; native --inspect passed through the PS5 x86 trampoline.
The v35b binaries and existing friend package are retained. Cloud unchanged.

Validation: complete x86/x64 builds; 71/71 native CTests; all five private native
rig replays for old and new solvers; actual native observer inspection. No new
managed protocol behavior. Headset/observer acceptance and two-VR-player haptic
feel are still required before publication or promising complete clipping fixes.


## Same-PC cloud flat observer setup (2026-09-25)

The owner requested reuse of the existing vrtester account for a flat observer
while the primary client runs in VR. Native --observer-profile now accepts an
explicit validated --join-server HOST --port PORT as well as --join-local; it
still requires flat mode, redirects Documents before native main, and holds
the exclusive observer profile lock. Primary duplicate guards are unchanged.
Managed Join.CreateLaunchInfo builds and tests the actual process arguments;
remote observers have separate settings/bridge ports and receive-only voice.
URI links still cannot supply profile paths.

Private checkpoint: local/multiplayer-lab/checkpoints/v35b-cloud-observer. It
retains the exact v35b client DLL and changes only the native launcher. The
owner/friend v35b runtimes and rollback are unchanged. Private cloud observer
runner and Play-Cloud-Observer.ps1 reuse profiles/ObserverDocuments and
config/ObserverVR.ini; the desktop shortcut is BF2142 - Flat Observer.
Start the primary VR client first; close the observer before restarting VR.
No headset/game restart was performed during setup; the user's open game
was left running. Actual two-client cloud admission/visual IK remains pending.

x86/x64 builds, all 70 CTests, 279 managed checks and the staged native
--inspect pass. Inspection does not launch a game or contact the server.
Private OBSERVER-QUICKSTART.md documents account/focus/recording steps.

## Cloud game server resumed at owner request (2026-09-25)

The existing server task was restarted. Verified native game and addon bridge
listeners, authenticated RCON with 64 slots/voting enabled, and 62 bots. The
server is ready at its existing address; no AWS configuration was changed.
Private current status: local/cloud/bf2142-vr-test/SERVER-STATE.md.
Owner asked about a flat observer while in VR: an outside player with the v35b
addon can test it. A second same-PC instance requires the isolated observer
setup/account; the standard cloud launcher is not a dual-instance launcher.

## Cloud game server paused at owner request (2026-09-25)

After the launcher update the owner agreed to pause the idle game server.
No humans were connected. Native exit acknowledged; the owned background task
was stopped, and zero game/addon processes or sockets were verified. The AWS
instance remains running; spending/deletion safeguards are unchanged. Before
the next multiplayer test, start the existing BF2142VR Test Server cloud task
and check readiness. Local client launch does not resume the cloud service.
Private status: local/cloud/bf2142-vr-test/SERVER-PAUSED.md.

## Private v35b remote weapon correction candidate (2026-09-25)

Owner clarified the pistol looks okay, the knife stays in native animation,
and the assault rifle is very buggy. These are explicit reports. The earlier
headset-off movement around 2:30 is intentional and excluded.

The outgoing anatomical palm now cancels the captured animated wrist with an
exact inverse within the existing near-rigid acceptance bound. A regression
reproduces the prior transpose multiplying a valid small native scale error
until the canonical knife palm fails packet validation. Shared
InverseAnimatedTransform contains the former InverseAnimatedBone math; the
receiver wrapper retains its existing behavior. Local rendered hand solving,
head/torso IK and server gameplay are unchanged.

Receiver knife/knife_unlock, eu_ar_rifle/as_ar_rifle and support-crate mesh1
roots now use the already-transmitted item pose. Mesh2..8 retain their native
relative animation; mesh9..16 are body equipment and must NOT be moved with
the item. Pistol/unknown-mod and unlike-bot mirror bindings stay unchanged.
Native selected-weapon identity must match real relays before applying a pose.
A per-actor cache restores only exact previous mod arm/item writes before the
native animation callback, so skipped/partial native LOD updates cannot reuse
stretched IK as authored limb lengths. Cache is allocated once per tracked
actor; unrelated native changes, head, torso and gear are not restored.

Saved at local/multiplayer-lab/checkpoints/v35b-remote-weapons/runtime.
Full x86 and x64 presenter builds pass, all 70 CTests pass, and all five private
native rig replays pass. Owner then authorized launcher activation: cloud
Play-Cloud.ps1 now defaults to v35b, with explicit -Build v35 rollback. Desktop
BF2142 VR / Multiplayer Test and the SteamVR Launch-VR-Dev.cmd entry point use
the cloud launcher; a Previous Build shortcut retains v35. The cloud service
is unchanged. Private dist/BF2142-Flat-Playtest-v35b.zip contains the signed
revision-3 flat update. Its exact EXE passes the v35-to-v35b offline upgrade,
all 13 file hashes and zero-download check. Both sender and receiver need the
new DLL for the complete fix; visual acceptance is still pending.

This candidate includes the saved v35a finger work. It is a private candidate,
not a visual acceptance claim or a new public/friend release. Confirm knife
tracking, raised rifle attachment/arms, left crate and retained pistol behavior
with actual sender/observer views before calling those reported bugs resolved.

## Private v35a receiver and voice investigation (2026-09-25)

The owner's external flat tester confirmed remote IK visually in a Streamable
clip (e0xcjz). Empty hands retained weapon-grip fingers. Packet v4 already carries
10 curl values; the remote solver ignored them. PoseRemoteFingers now applies
those values after each successful arm solve. 3p_setup.ske has ring/index/thumb
chains at 21/24/27 (left) and 36/39/42 (right), with 3 bones each; it cannot expose
five independent third-person fingers. Native finger parent topology is checked.
New tests cover curl direction, individual hand/chain changes, lengths, held
contact, invalid data and transformed frames. All 70 native CTests and x86/x64
builds pass; detailed and collapsed private 80-bone captures also pass. Stage as
v35a-remote-fingers for later acceptance. Do not replace a running user's DLL.

Cloud voice did send real microphone packets in both directions: the owner's
VR session counters reached 6525 sent, 1111 received and 1154 played, with WinMM
input/output errors zero. Both clients had encrypted addon admission concurrently.
This is transport/decode/playback submission, not confirmed audible speech.
The owner heard no prox audio. Windows default input/output resolve to Steam
Streaming Microphone/Speakers. Muting false, voice enabled, volume 1, threshold
-40 dB. Foreground/tracking gating paused capture while the owner used desktop.
The new flat session received 71 voice frames but sent none during observation;
Windows defaults still target Steam devices. Asked which desktop mic/output to
use; do not assume Kanto speakers vs HyperX headphone jack. Do not change global
Windows defaults or send synthetic audio to the cloud without a focused test.

A later expanded review of the entire external clip timeline (one-second
samples plus close-ups) confirms the support crate floats near the waist while
the left hand moves (0:28-0:36), and empty-hand fingers stay fixed (1:35 onward).
IMPORTANT OWNER CORRECTION: around 2:30 the headset was removed and deliberately
moved around. Exclude that head/neck strain from the bug list; do not clamp or
change head/torso IK based on this intentional movement. The owner subsequently confirmed knife fallback and buggy rifle IK; pistol
looks okay. Exact angular corrections still need sender-view comparison. The v35a finger patch does not fix crate attachment. Private
timestamped findings are in working/ik-full-review. The popup at 2:47 is ordinary
"Menu loading", not a voice setup popup.

The desktop briefly became unusable. By the stop attempt the game and presenter
had already exited; launcher reported game exit 0. Owner confirmed recovery.
Do not claim a crash diagnosis or that the stop command killed a process. Owner
then requested flat play and rejoined the same cloud through Play-Cloud.ps1 flat.
Review browser is isolated/muted and should be closed when clip review finishes.
Private diagnostic tools: Read-AudioDevices.py, Observe-Lab-VoiceDetail.py.

## Private v35: live cloud connection reached (2026-09-25)

Read BF2142_COMMUNITY_HOSTING.md. Real native ClientCommand challenges now pass
both the local lab and the owned cloud Windows dedicated host. A real flat
client joined Suez Titan over the public internet; TLS pinning, native ownership
proof and encrypted addon admission succeeded. This is not yet a two-person
internet IK/voice acceptance or a Quest microphone test. The public release is
still alpha.1; v34 remains the selected rollback. Native x86/x64 builds and 70
CTests passed earlier; the current managed suite passes 258 checks.

The cloud originally crashed at dedicated RVA 0x38609: the legacy backend init
failed and its subsequent getter returned null. sv.internet 0 stayed alive;
applying the documented OpenSpy dedicated patch's five domain replacements to
a backed-up private executable made sv.internet 1 work. Never ship that game
executable. Preserve native signature checks after third-party patches. The
cloud runs as a limited local user, with only SeBatchLogonRight added for its
startup task. RCON, native pose/audio, and proof sockets remain localhost-only.

HostSetup now forwards launcher diagnostics, reports nonzero native exits, and
waits for the RCON exit acknowledgement. Closing the RCON socket immediately
after writing exit could lose the command. These latest managed source fixes
pass tests but are NOT yet deployed to the running cloud NativeAOT helper.
Manage-Maps.ps1 provides List/Change/Next through localhost RCON and validates
that a selected map belongs to the live rotation (co-op, Conquest, assault,
assault lines or Titan). Runtime next-map/current
indices confirmed a change to Minsk. Cloud starts on Suez after a normal restart.

Owner chose the quicker Conquest/co-op bot test. The cloud has 64 slots and
62 actual bots verified in Suez gpm_coop 16. All 20 installed maps are in the
saved/live rotation and map voting is enabled. Only Suez, Belgrade, Cerbere,
Berlin and Verdun stock co-op layouts have bots; Wake and other modes are
human-only. Layout 16 is separate from the 64 player-slot setting. Both the
owner and an outside friend have joined the native game; their two-addon
IK/voice verification is still pending. A map change back to Suez was requested
after the rotation advanced to Gibraltar. Do not resize the 2 GB instance.

Private cloud state, connection fixtures, credentials, crash dump and remote
scripts are outside this repository under the local cloud workspace. A private
fixed-host development runner uses the existing v35 runtime for the successful
cloud join; it is not the signed public downloader. The private flat join EXE now bundles one signed flat ZIP using BFJOIN02;
OfflineBundle verifies its descriptor, exact archive and every file before use.
It avoids HTTPS package hosting for this first friend's test; offline updates
are manual. The normal BFJOIN01 online updater remains available. Flat package
verification does not require a headset. Exact bundled files/install are checked,
while the outside friend's first addon launch and IK/audio are pending. Full VR
packaging, setup payload.json and HTTPS/public distribution remain pending.
The one-month AWS deletion schedules, cost emails and five-minute traffic guard
are configured; SNS traffic-email subscription confirmation remains unconfirmed.
No cloud resize or additional paid instance was performed.


## Owner priority: internet play and automatic flat-client setup

The owner need not stay in the headset or keep the local match open while we
implement networking/hosting. Batch the remaining Quest mic/native squad-radio
check later. Download-on-join is an explicit hosting requirement: build toward
one-time helper/launcher setup and automatic versioned addon download + join,
with a lightweight flat IK/proximity package and no SteamVR requirement.
A completely unmodified client cannot presently fetch/load our injected DLL
from a server join; do not claim that bootstrap is solved. Reclamation Hub
integration is a possible route, not an agreed integration. See the new internet
hosting and automatic flat-player setup sections of BF2142_ROADMAP.md.

## Private proximity voice candidate (v34)

Read BF2142_VOICE.md and BF2_POST_ALPHA_UPDATES.md. Preserve the selected v33i
rollback. This adds a separate Opus/WinMM loopback voice relay on 17569, admitted
only after the existing pose/subscription session matches the live native owner.
The native team getter is player vtable +0xf8 -> server RVA 0x13e930, verified
against `mov eax,[ecx+0xd8]; ret` and the native Python `pmgr_p_get("team")`
callsite at 0x106ebc. Unknown team denies teammates-only voice. No native writes
or changes to pose protocol v4/presenter IPC v24 were introduced for audio.
Host `[Voice]` defaults: everyone nearby, 20 m; disabled unless explicitly enabled
in BF2142VR_NETWORK. Input capture requires live state/focus and (in VR) fresh
focused controls. Radio pauses proximity. Settings UI names its mute as proximity
only; native V/B remain available. Observer script uses independent preferences
and receive-only mode to avoid two microphones on the same PC. WinMM/Opus live
on a separate worker and no raw microphone recording is made. The codec source
and licenses are vendored, but no new game assets are included.
70 CTests, x86/x64 builds, real WinMM output and stereo GPU smoke passed.
Two real native clients passed the proximity path on Suez: synthetic 440 Hz
through VB Cable -> primary WinMM capture -> Opus -> live native server relay
-> observer decode -> WinMM output; 62 sent/received, 63 played including PLC,
zero device errors. No microphone recording was saved. Quest microphone/radio
and audible headset quality remain pending. The primary INI has been restored
to communications-default input/output for its next launch; the still-running
desktop process retains the temporary cable input until restart. Observer uses
receive-only mode and its separate speaker output preferences.
Public hosting remains separate work: both custom transports are loopback-only;
external authentication/encryption and deployment require their own design/test.
Roomscale walking is explicitly deferred at the owner's request.


## Physical shoulder radio candidate (v33i, private)

The owner requested an actual weapon-like hand grip/press on an upper-left
shoulder radio. ShoulderRadio now requires a new left squeeze inside an 11.5 cm
contact region, then holds until release, tracking/focus/menu/owner loss, recenter
or pulling 27 cm away. The thumb depresses a 4 mm switch in about 60 ms before
native V is held. An entry with squeeze already held cannot activate voice.
The old v33h left-trigger/stick chord is removed; ordinary sprint is unchanged.
The body anchor remains available when holster visuals are disabled, but its
weapon selection outputs are explicitly suppressed in that case.

The radio is procedural geometry, drawn once per eye before native HUD isolation,
with the native close-weapon projection and depth testing. Do not replace it with
an overlaid body-prop bitmap: fingers must occlude the casing. D3D state is restored;
no proprietary meshes or textures are added. The left palm has a dedicated fixed
contact pose, disables support/pistol bracing, and has independent finger curls.
A bounded thumb-only FABRIK solve reaches the moving switch while preserving bone
lengths; an unreachable contact retains the normal grip. NativeHands publishes
radio and controller data together with the same generation/freshness gate.
The right weapon/hand are not retargeted. Existing held crates have priority.
Remote palms already relay the shoulder pose; remote radio meshes/fingers do not.

A new left haptic counter requires shared presenter IPC version 24. Always deploy
matching x86 client and x64 presenter. Multiplayer pose wire protocol stays v4.
69 Win32 tests, x64 build, actual D3D9 stereo/depth/state smoke (MSAA on/off), and
16 captured native first-person hand frames verify mechanics. Captured thumb
contact error is below 0.5 mm without changing the right-hand/weapon output.
Actual shoulder ergonomics, native in-game depth appearance and audio delivery
still need the combined headset/observer check. This is a private candidate.

The private lab server had sv.voipEnabled 0; backed-up config now enables native
VoIP. Both profiles already had VoIP and push-to-talk on. Runtime setting readback
became 1 but service ports did not appear: a server restart/audio test is pending.
No microphone device or open-mic preference was changed. Native stock V supplies
squad talk; stock B/commander routing is not remapped. Requested voice-activated
proximity with mute controls needs a separate audio path and is not implemented.
The next milestone is actual native squad audio between the two clients, followed
by no-button proximity and commander/leader radio design. Addon auto-download is
still future work. Do not claim working voice from the radio's visuals/input.


## Explicit standing recenter and remote head rotation (v33g, private)

The user returned from OBS, recentered the camera, and remained natively crouched.
StereoSession previously retained an independent standing Y. Right-stick click,
Home, presenter explicit recenter and Calibrate Standing Height now also capture
current LOCAL Y and reset PhysicalStance's debounce/toggle state. Zero is a valid
session calibration; saved preferences cannot overwrite an explicit session reset.
Automatic focus/spawn camera recenters do not redefine standing height. Native
prone exits through the existing bounded toggle/retry policy, not memory writes.

Protocol v4 already relays the head in soldier-local coordinates. ApplyRemoteNetworkPose
now solves its orientation independently from arm reach/binding. The supported
3p_setup rig has head 47, with face descendants 48-63; verify every id/parent
before those writes. Keep bone 47's native position and transform the entire
face subtree by its old-to-tracked rotation. The checked native setup has a
neutral +X right/+Y up/+Z forward head basis. Preserve neck, torso, legs, item
attachments and arm output. Local rigs, stale poses, mounts, dead actors and
unknown face topology retain native head animation. No tracked positional neck
or torso solve, facial gestures or networked finger retargeting is claimed.

68 Win32 CTests, x64 build, five captured native rigs (with a detailed palm
reference for collapsed LODs), and the basic real D3D9 stereo/pause fixture pass.
Tests cover LOCAL-origin recenter from crouch/prone, zero-height calibration,
head yaw/pitch/roll, invariant face offsets and neck attachment, and independent
head/arm fallback through the actual remote animation callback. The user's
previous v33f recording shows actual human-to-human arm motion and empty hands,
then weapon redraw, but also the reported crouch fault. Head rotation in v33g
remains a new, unaccepted headset-visible candidate. The saved v33f is rollback.

Voice is now requested: voice-activated proximity plus a physical squad radio.
The stock dedicated installation includes its VoIP component and server manual;
first verify native squad delivery, then add proximity. Addon delivery is later.


## Receive-only observer and remote empty hands (v33f, private)

The launcher now supports --network-observer: native flat rendering/input, no
OpenXR session, synthetic controllers or local VR controls. The matching
protocol-v4 server accepts a separate authenticated Subscribe lease and relays
other players' poses to it, including while the observer is unspawned. Leases
expire after one second, have sequence/session/port checks, and never provide
tracked fire, movement, snap or throw authority. This is still loopback-only.

For two clients on one PC, add --observer-profile "ABSOLUTE ISOLATED DOCUMENTS"
and an explicit --join-local PORT or --join-server HOST --port PORT to the
flat observer. Before native main resumes, this child's
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


This is the primary continuation guide for human developers and AI coding
agents taking over BFVR. It explains what the major pieces do, which behaviors
must be preserved, and how to verify changes. It is intentionally more current
and task-oriented than the chronological `devREADME.md`.

## Desktop mirror and sleeping headsets

The owner could not see/click Login with the headset off. The presenter had
consumed one frame and remained in SYNCHRONIZED, covering the live native UI
with its initial partially drawn login. Hiding only BFVRDesktopMirrorCanvas
exposed the actual game, which was already loading the Titan server. Saved
credentials were intact; this was a stale desktop preview, not an auth failure.

Poll IsSessionVisible even when no new source request arrives. Hide the child
canvas outside VISIBLE/FOCUSED; waking waits for a newly accepted source frame
before showing it. Create/resize never force a stale canvas visible. Retain
normal last-frame recording during visible source gaps; do not restore flat
gameplay rendering at the XR fence. The owner remains on v33c while this
presenter-only follow-up is staged; no live restart is needed for development.

## Objective marker preference: v33d

The owner identified the floating Titan/silo symbols and explicitly requested
that they be hidden. Native Left Alt (c_GI3dMap) removed them in the live v33c
match; a capture confirms the minimap and top Titan health bars remain. This
accepts hiding the markers, not the earlier stereo-projection correction.

The next client defaults HideWorldMarkers=1 and uses the verified native
culled-point sentinel (0,0,-1,0) for type-3 glued world-marker projections,
including both icon positions and scope calls. Preserve the original call,
return pointer, other tag types and unglued labels. The profile additionally
verifies native sentinel bytes at RendDX9+0xf38cd. Only enabled VR hides them;
HideWorldMarkers=0 retains the optional v33c projection candidate. No game
archives, minimap data, server objects or native player-name code are changed.

## Titan direction-marker candidate: v33c

The owner reached Suez Titan on the actual v33b VR client and dedicated server.
The reported duplication is the in-world Titan arrows, NOT the HP bars/top HUD.
CPU eye/UI captures showed clean HUD isolation; do not change the HUD capture
or menu panel to address this. Native nametags use RendDX9+0xef210 to project
3D-map items, with independent +/-0.65 NDC edge clamping. Equal clamped NDC on
asymmetric OpenXR eyes describes different directions.

v33c hooks that signature-checked projection, preserving its native call and
return pointer. Only type-3, glue-to-edge markers inside an ordinary stereo eye
are corrected. Derive one head camera from the current eye/request, choose the
edge direction in the common eye frustum, then project the same 100-metre cue
into each eye. In-view markers retain their actual world point. Scope/outside-eye
calls, ordinary player labels and unknown profiles use the native result.
Native visibility/selection remains in charge; no game archives are changed.

66 CTests and ten D3D9 GPU fixtures pass, including asymmetric-eye ray agreement,
rigid-frame invariance, native fallback/return handling and identical reconstructed
head frames across both GPU eye passes. x64 also builds. The renderer signatures
were verified read-only against the live process. Actual v33c marker appearance
and current-build bot-mirror arm IK still require in-game acceptance. Protocol v3
is unchanged; the existing v33b server can remain running. Accepted v33b is the
private rollback. No public release or SP experiment has been changed.

## Multiplayer movement and visible weapons: v33a/v33b

v33a fixes the invisible hands/weapon report. NativeRender copies the current
world-eye matrix into the weapon camera before first-person drawing. It ALREADY
contains HMD motion/IPD. The pre-render weapon camera can still be at (0,100,0)
or an older spawn. CameraSetterHook must accept this native copy for normal
motion hands; retain explicit overrides for world, scope and legacy trackedWeapon.
A fresh real dedicated spawn and move/turn capture show hands and gun with v33a.
The old note calling this desktop-only was incorrect. The XR fence fix stays.

v33b additionally addresses movement diverging from the recoil-free view. In a
native desktop match, W traveled at -0.188 degrees while the rendered heading
was -4.464 degrees: native view yaw retained 4.274 degrees of accumulated recoil.
NativeComfort's movement getter wrapper replaces only the exact walking/sprint
basis read: client getter 0x189000, return 0x18b033; server getter 0x12bab0,
return 0x12d7d6. Verify entry/tail/call bytes. Native getter side effects are kept,
all other calls return its original pointer, and no camera/body memory is changed.
The temporary level matrix applies tracked movement yaw minus removed recoil.
The keyboard/left stick then use ordinary forward/strafe keys without applying
tracking yaw twice. Preserve stock speed, sprint eligibility, pose/fire and IK.

Client ownership is exact live local infantry plus weak identity, focus and
150 ms tracking freshness. Menus, recenter, reset, mounts and tracking loss
invalidate it. Server requires a fresh authenticated matching-weapon human pose,
foot state and its input thread; flat players and AI retain their native basis.
Protocol v3 appends a bounded movement offset and MovementValid flag (616 bytes).
Use a matched client/server pair; v2 checkpoints must be rolled back together.
65 CTests, x64 build and 10 real D3D9 GPU cases pass, including 720 recoil/turn/
view agreement cases and native caller/owner/freshness guards. A fresh v33b
dedicated spawn shows hands/gun. After firing, unobstructed W travel measured
48.550-48.561 degrees against the 48.555-degree comfort heading (native input
50.470 degrees, removed recoil 1.915). Stair/collision slides were excluded.
On 2026-09-24 the owner accepted the actual v33b VR multiplayer session:
"works great." Individual movement settings and sprint cases were not separately
reported. Current-build mirrored arm IK, Titan gameplay and two real VR clients
remain pending. A simulated head-turn walk was interrupted by focus loss; no
result is claimed for it. Private validation.json retains these distinctions.

Private checkpoints remain outside source. v33a is a separate weapon-camera-only
checkpoint, v33b adds movement. Normal/public v30 and SP experiments are untouched.

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
completion and two real clients still require testing. The viewmodel visibility
issue reproduced with both v33 and the saved v32a DLL; see the v33a fix above.
Do not claim headset visual validation from the desktop result.
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

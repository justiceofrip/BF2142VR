# BF2142 native controller hands (v13 development)

The user has played v11 with tracked hands in a headset and reported the
remaining menu, aim, scope and hand issues. The v12 run confirms menus and
gun-directed firing, with a bad left grip, rejected optic and renderer crash.
v13 is the follow-up and is not yet headset-verified. BF1942 adapters are unchanged. The new adapter lives in src/bf2142/NativeHands.cpp; its pure
model-space solver is HandPoseMath.cpp and reuses BFVR ArmPoleVectorMath.

## Native ownership and update order

The local player comes from the signature-checked renderer PlayerManager global
at RVA 0x221a58, manager+0x6c. Player+0xcc is a weak soldier reference (+4 is the
object). Player+0x88 is a camera reference and MUST NOT be used as the soldier.
The soldier vtable is exe RVA 0x566150. Its active inventory index is +0x218;
array begin/end are +0x234/+0x238. Current firearm vtable: exe RVA 0x56cd88.

The first-person skeleton is soldier+0x2c0, animation controller +0x2c4;
third-person equivalents are +0x2c8/+0x2cc. Mode +0x2f0 must be zero. Reject the
vehicle/attachment path +0x28c. Skeleton+0x10 holds final 64-byte matrices;
+0x14 is count (70); metadata +0x08 has 0x24-byte entries with signed parent
index +6. Validate both indices and parent topology before writing.

First-person finalizer exe RVA 0x1ee980 returns with ret 4. It calls native
SkeletonTransform (0x4b8880). Soldier animation update (0x1f06b0) subsequently
binds bone 54 to the active weapon via vtable slot 58 and updates animated
mesh parts. A live bounded observer confirmed this update executes in the
render path. Run native animation first; never accumulate controller transforms
on previously modified poses or write source animation/asset records.

| Arm | Upper | Elbow intermediate | Lower | Ulna | Wrist/fingers |
| --- | --- | --- | --- | --- | --- |
| Left | 3 | 4 | 5 | 6 | 7 / 8-27 |
| Right | 29 | 30 | 31 | 32 | 33 / 34-53 |

Bone 0 is camera, 1 is root, 54-69 are weapon mesh parts. Preserve camera/root.
Rigidly carry authored wrist/finger/weapon relationships with the solved pose;
solve upper/lower joints anatomically. Grip position owns the wrist, aim
orientation owns the gun. Left squeeze within 20 cm of the foregrip acquires
support; 30 cm releases it. Untracked left hands retain authored support.

## Native firing

Exe RVA 0x1f2bb0 takes two matrix pointers and inherited velocity (ret 12,
projectile pointer result). v11 incorrectly treated the first as a barrel
record and replaced only the second. Disassembly of its call to CreateProjectile
(exe RVA 0x1f18f0, ret 8) establishes that the FIRST matrix owns the main shot.
The second is used in a secondary projectile path. The main matrix getter at
0x1f1a20 normalizes the basis and applies template barrel offsets.

v12 hooks the matrix getter at 0x1f1a20 (fire-interface slot +0xd4, ret 0),
which runs BEFORE driver 0x1f31d0 calculates velocity. Return a thread-local copy
mapped as nativeLaunch * inverse(nativeCameraWorld) * solvedWeaponWorld,
retaining native local offsets/deviation. The driver then computes native muzzle
speed times the corrected forward vector, adds player movement velocity and
applies its normal clamp. Correcting only the constructor matrix would still
leave bullet velocity aimed along the old camera. Do not transform that result
again at the constructor, and do not rotate the inherited player velocity.
The constructor's second matrix receives the gun pose for the secondary path;
main creation, spread and the already-computed velocity remain native.
Receiver+0xc must be the exact active weapon; receiver+0x10 must be its native
fire interface (vtable RVA 0x5703c8), and weapon+0x1b4 must point back there.
Solved bone 54 multiplied by soldier world supplies the gun pose; unmodified
camera bone 0 times soldier world supplies the camera frame. Require the same
accepted controller generation, current owner, focus and age. Reject nonrigid
poses or an implausible native launch offset rather than guessing.

Only the observed firearm class is enabled; unsupported tools/vehicles retain
native handling. Multiplayer propagation is not verified.

## Grip stability and provisional optic

Capture the right wrist relative to weapon bone 54 as before. The off-hand
must settle separately: a forward, plausible support pose must remain within
1.2 cm and roughly 3.6 degrees for 250 ms, at least 750 ms after equip. A gap over
150 ms restarts that interval. Do not let a static rearward deploy/reload pose
qualify. Freeze the accepted left binding, including across subsequent reloads;
reset on soldier/skeleton/weapon changes. The initial under-barrel correction
is therefore based on the gun's own settled animation, not a made-up offset.
Continue deriving fingers/animated parts from fresh native matrices. Physical
fit still needs user feedback; right-hand aim and firing are preserved.

Historical v12 prototype (inactive in v13): right grip suppressed native flat
ADS input and activated an initial
per-eye red-dot optic. Its generic lens anchor is 5.5 cm up and 8 cm forward
from the weapon origin, radius 3.4 cm. Rays point to a 50 m bore zero and clip
against the aperture; dot position changes correctly for each eye. Rendering
occurs in the captured eye pixels without extra native Render calls. It has no
magnification, depth occlusion, per-weapon placement or native ADS accuracy
bonus. The user rejected it. v13 restores native right-grip ADS and removes the
prototype from the active render path. Proper VR scope rendering remains open.

## Verification boundary

Both architectures build; all 41 CTests and renderer/stereo/MSAA GPU checks pass.
Tests cover wrist/finger/item relationships, rotated guns, animation-independent
grips, support hysteresis, reach limits, invalid poses, main projectile mapping,
optic aperture/parallax/colors, control mapping and pause-frame publication.
v11 logs show the native hand/fire hooks executing in a user-played match; that
exposed the wrong firing argument, not proof of correct aim. The v12 user run confirms working menu input and shot direction. v13 changes
still need a headset check; keep v12 available. The separate renderer-query
crash guard passed an isolated real-binary lookup/detour probe, not a live match.


## v16 optics integration

The new GunOptics/NativeOptics path replaces the inactive generic prototype for
five measured stock rifle/sniper profiles. It reads the accepted solved weapon
world frame without changing any hand binding, solver or firing code. Native
right-grip ADS remains active; only the supported local zoom visual LOD changes.
Lens-only magnified rendering and its current limits are documented in
[the port status](BF2142_PORT.md) and [the developer handoff](AI_DEVELOPER_HANDOFF.md).
The old WeaponOptic helper remains inactive; do not reconnect its generic ring.


## v17 supported ADS hand shape

The accepted v16 optics remain. The new AdsHandPose cache records bones 2..53
relative to weapon bone 54 only after the non-ADS foregrip has settled. During
supported native ADS, and for 350 ms after leaving it, restore that holding
shape against the current native weapon frame before SolveTrackedHands. Root
and camera bones 0/1 and animated weapon parts 54..69 stay native. Reset the
cache on soldier/skeleton/weapon changes; do not settle bindings from an ADS
pose. A missing cache falls back to native animation. Wrist targets, support
hysteresis, gun-directed projectile mapping and v14 comfort are unchanged.

Automatic ADS uses the same solved world weapon frame and measured lens view
as v16. It requests the verified native SetZoom transition, preserving native
ADS gameplay effects. It does not simulate zoom by magnifying the headset.
47 CTests cover native ownership/transitions, stale guards and hand shape;
headset confirmation of the v17 animation change is still pending.


## v18 recovery supersedes the v17 pose override

The user reported displaced hands in the v17 headset run. Remove AdsHandPose
from the active NativeHands and client build, restore unconditional v16 binding
updates, and pass fresh native animation into the accepted solver. The isolated
experimental helper/tests remain as history, not a live stabilization feature.
NativeHands::Apply was compared with the saved accepted v16 implementation and
is identical apart from comments/whitespace. The release/launch mapping and
comfort camera are unchanged. Automatic ADS is disabled by default and in the
recovery configuration; right grip and the accepted physical lens optics remain.
This restores a known path rather than claiming the precise hand-only failure
has been isolated in a live A/B test.


## v22 body inventory and free-hand fingers

The accepted wrist/weapon binding and shot mapping remain. Free-left-hand
fingers can extend from the native authored curl; while supporting the weapon,
all finger transforms remain authored. The right weapon hand is unchanged.
The actual owned `Common/Animations/1p_setup.ske` verifies five four-bone chains
under wrists 7 and 33. Their order is thumb, point, ring, index (middle), pink.
Runtime checks validate all forty parent/index records before enabling fingers.
Curl processing is atomic and rejects invalid chains/lengths/values; rotations
preserve segment lengths. Touch supplies index/thumb state; squeeze supplies
remaining fingers. Fresh-sample smoothing avoids abrupt capacitive transitions.
This is controller-driven animation, not independent finger-joint tracking.

BodyInventory consumes right-squeeze rising edges within current available
slots: item 3 back, 2 right hip, 1 chest, 4 left belt, 5 rear belt, 7 front belt, 6 right rear belt.
The yaw-only body anchor has a turn dead zone so looking toward a shoulder does
not immediately move the slot away. Focus, stale samples, menus, death and
vehicle state cancel selection. Native input receives a bounded 120 ms number
key; no inventory objects or network state are written. Existing wheel/keyboard
selection stays available. Unsupported native alternate fire remains available
outside grab zones. This is a first implementation of physical slot selection;
it does not simulate ammunition magazines or weapon drop physics.

BodyEquipment reads an explicit private pack generated by ExportBodyEquipment.py
from the owner's stock Weapons_client.zip and optional expansion textures.
Its 58 complete low-detail world models are independent of the repaired held
first-person models. Missing stock textures are recorded and skipped. Current
inventory names resolve exact mesh names, with a reviewed `_rifle` suffix
fallback. Visible props follow physical slots, hide the held item and tint hover.
Rendering is bounded to 640 pixels across per eye and has prop-to-prop depth;
there is no shared native world/hand depth, so clipping at walls or with the held
weapon remains possible. The pack is proprietary and must not enter the repo.

Multiplayer players still see native game animations. Remote controller poses
are not sent by this implementation; network-visible IK/waving remains open.
Vehicle cockpit reconstruction remains open and requires vehicle-specific art
and seat/camera work. Do not describe this development batch as a finished port.


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

## v25: accepted feedback and new interaction policy

The owner accepted v23 grip/body placement and the completed v20 exteriors.
They rejected v24 face fading and gesture grenades. Motion knife attacks work,
but the native icepick blade orientation must be reversed. v25 follows that
feedback; it is a private test checkpoint, not a public release.

`FlipKnifeAtHandle` rotates mesh parts 54-69 around the measured local handle
pivot (0, .006, -.100 m); it never rotates the solved wrist/fingers. `WeaponGrip`
owns the right-squeeze rising-edge toggle. Holstering parks only weapon mesh
parts outside the eye frusta and lets both wrists/fingers track freely. No
world pickup/drop is created and native inventory remains selected. Native
projectile creation is suppressed for the exact local owner while holstered
and until a fresh attachment pose exists after drawing. Menus, tracking gaps,
owner changes and body-slot acquisition have explicit policies/tests.

`RemoveInteriorBackfaces.py` operates on a privately generated completed
weapon archive. It verifies the provenance of mirrored triangle/index pairs,
then removes only artificial interior-facing copies inside opposing rigid
shells. Every original/completed exterior triangle, vertex and UV is retained;
transparent materials and world LODs are preserved. It creates no public assets.
The private v25 selector recognizes both v25 and v20 for reversible rollback.
The first-person near plane is 6 mm; no whole-gun opacity effect remains.

Frag throws use the native primary action again, retaining stock velocity,
player-motion inheritance and animation delay. The cyan arc is a short aiming
guide from the verified fire frame/speed, not a collision or landing prediction.
Knife swings/stabs retain v24's bounded motion policy. MotionActions=0 restores
trigger knife attacks. Gesture grenade priming/release has been removed.

`VrControlsMenu` is composed into the existing world-space UI texture and uses
exactly the same laser/canvas mapping as deployment. Its events consume input
before the native UI; held triggers cannot click through on close. Insert is
the desktop shortcut, and a VR CONTROLS tile appears on native menu panels.
Preferences use the current BF2142VR_CONFIG path and apply immediately. The
menu displays save failures without discarding the live preference change.

Snap turning requires neutral rearming, ignores stale tracking, and rotates
the tracking reference around the current head. Eyes, hands, aiming and
head-relative movement share the same transformed reference. Smooth turning
retains the prior native input path. Recenter restores the native facing frame.

PhysicalStance classifies standing/crouch/prone from head drop relative to the
standing calibration: crouch enters at 30 cm/exits at 20 cm; prone enters at
88 cm/exits at 72 cm. A 180 ms dwell rejects threshold jitter. Native +0x26c
stance is read only after the infantry ownership/profile guard; Z is a short
pulse for prone toggling and LeftCtrl is held for crouching. Actual native stance
is checked before each retry. No stance action runs in menus, vehicles, on death
or without fresh focused tracking. Stock Ctrl/Z infantry bindings are required.

PhysicalCameraHeight shares the same standing world Y origin between eye and
hand composition, compensating the native lowering so physical crouch is
applied once. Calibrate only a stable local first-person offset 0.15..1.1 m above
the soldier root, after 300 ms; deployment/spawn cameras must not become the
baseline. A new soldier invalidates that baseline. Until a standing view has
settled the native camera is retained. Standing-height calibration should be
performed upright; turn Physical Stance off for seated/button play.

The crosshair lookup validates the native HUD variable-manager profile and
resolves CrossHairColorAlpha by exact name through its guarded maps. Zero only
during native HUD drawing and restore immediately afterward. No HUD region is
erased and no game archives are patched for this setting. Native hit indicators
that inherit their parent crosshair alpha may also be affected; VR lens reticles
are independently rendered. The procedural main-menu room supplies binocular
parallax; the original Flash menus remain a floating interactive panel.

Validation: Win32 client/launcher/translator and x64 presenter built; complete
52-test suite and 11 GPU fixtures pass. Desktop game verified laser-driven
settings persistence and native stance 0 -> 1 -> 2 -> 0. Final headset/controller
feel, close-eye stock visibility and all weapon coverage still need acceptance.
No manual reloads, bare-hand finger tracking, replicated remote IK, vehicle
cockpit reconstruction or new multiplayer compatibility claim is included.

## v26: knife isolation, cupped handguns and presentation polish

The owner's v25 feedback: keep the general hand setup unchanged; knife orientation
and native off-hand animation still fail, the revolver needs a palm support grip,
the grenade guide does not align, and menu room/SteamVR branding need polish.

The owned stock knife inspection resolves the former direction assumption:
negative mesh Z is the thick handle, positive Z the thin blade/tip. KnifeInGrip
centers (0, .006, -.100) on the controller grip without a 180-degree flip. OpenXR
-grip-Z points little-finger to index/thumb; after conversion that is D3D +Z.
ControllerHandPose derives that frame from verified finger-root anatomy, with
palm depth toward the curled fingers. ControllerHandCache keeps a provisional
pose until the existing right-grip settlement accepts a holding frame. Then it
locks that local skeleton reference across weapon swaps; new soldier/skeleton
ownership clears it. This prevents caching an open first draw frame forever.

Only knife solving uses the fixed reference arms/fingers/clavicles. Its tracked
left palm depends on its own controller; missing left tracking uses a torso
rest pose, never the weapon transform. Native cameras/root remain unchanged;
parts 54-69 retain the existing owned attachment path. General rifle poses and
free hands for other weapons retain the prior solver. Knife motion attacks
remain right-hand swings/stabs; tossing/catching or transferring ownership is
not implemented and must not be implied by a visual attachment swap.

Exact template names eu_handgun/as_handgun enable close support. Left squeeze
plus <=14 cm palm separation acquires; >=21 cm or release disengages. Readiness
uses the settled firing grip, not the rifle-only foregrip detector. The left
palm cups the exposed left side/lower portion of the right grip. The barrel
keeps its right-hand direction; a tiny/zero palm baseline never defines aim.
An approximately 25 ms angular filter steadies small supported rotations, with
immediate release and resets on tracking gaps/fast turns. Translation is never
filtered and the final same weapon frame drives rendering and projectile aim.

TrackedFragLaunch is shared by LaunchHook and ReadNativeGrenadeTrajectory for
the exact stock frag only. This avoids applying camera-to-weapon mapping twice
when ThrownFireComp obtains an already moved weapon transform. Native launch
velocity remains speed * launch.forward + the owning infantry's movement. The
guide reads the exact physics getter's +0x78 velocity only after validating the
0x593aa0 vtable, +0x64 getter=0x2c5560 and lea/ret byte signature. Unrecognized
physics fails closed. Stock frag gravity modifier .55 remains the guide profile;
there is no wall collision/bounce/landing solver. The current-aim preview may
change during the stock .39-second primary launch delay. No gesture throw was
reintroduced and no ammo/inventory or movement velocity is overridden.

MenuRoomGpu shades the room at full output resolution using pixel derivatives
and integrated stripe coverage. It owns an offscreen non-MSAA target, restores
native targets/depth/state/constants/viewport (including an MSAA native target),
and commits the two completed eye buffers atomically. Device reset releases its
GPU/capture resources. Shader failure retains the native background and does
not repeatedly compile or run a costly CPU fallback. MenuRoom.cpp is the CPU
reference only; it is not called by the normal VR frame loop. No in-game world
anti-aliasing setting or floating-menu geometry is changed.

The explicit 0x40 producer bit selects Battlefield 2142 VR as OpenXR's title;
1942 remains the default for other producers. Register-SteamVR.ps1 is an optional
64-bit utility process using the installed OpenVR library. It registers only
bfvr.battlefield2142 and verifies the exact presenter's process key plus name and
image metadata. Start-SteamVRBranding.ps1 launches that helper hidden with a
bounded startup watch; registration failure never prevents game launch. The
actual logo stays in private local artwork, extracted unchanged from Menu_client.
API reference: https://github.com/ValveSoftware/openvr/blob/master/headers/openvr.h
Grip-space reference: https://registry.khronos.org/OpenXR/specs/1.1-khr/html/xrspec.html

Validation: 53/53 deterministic tests and 12/12 GPU fixtures, Win32 client,
launcher and translator plus x64 presenter built. Tests cover knife tip/handle
axes, stable full arms under an animated native source, provisional/settled
cache, no-tracking off-hand independence, pistol acquire/release/side/zero
baseline, aim filtering/reset, actual frag getter versus guide plus inherited
movement, and room pixel coverage/state/reset. Room GPU timing was about 3 ms
at 1280x720 stereo including readback. SteamVR registration was verified live
against the previously running v25 presenter. No v26 headset acceptance claimed.

## v27: shared turn heading, free fists and movement direction

The accepted v26 checkpoint remains unchanged. v27 removes the reference-only
snap-turn path. The verified getter call at game+0x18acf9 returns a horizontal
look delta, added at +0x18acfe and integrated into local/body heading at
+0x18adc5/+0x18aea4. NativeComfort adds one queued degree pulse only for that
caller and the exact live local infantry owner. It records actual recoil before
adding the pulse, so comfort excludes recoil but retains player turning. The
request is consumed once, expires after 150 ms, and is cancelled for focus loss,
recenter/menu/reset or owner change. No direct soldier-heading writes, mouse-sensitivity
calibration, per-eye input or independent virtual turn accumulator are used.

The hand solve receives the level comfort camera in skeleton coordinates as
its torso frame. Shoulder offsets and the elbow-pole solve run in that frame,
then transform back into model coordinates. Wrists remain at tracked targets;
held knife/gun bindings and camera/root bones are unchanged. Repeated full-turn
fixtures exercise rifle, knife and empty-hand rigs.

WeaponGrip treats a squeeze edge away from a slot as holstering only. Already
empty hands stay empty, and can squeeze repeatedly without summoning a weapon.
Body-slot grabs and explicit native equipment selection draw as before. Empty
hands use cached palm-relative anatomy; fingers bend toward the palm with
length-preserving joints. Squeeze controls the last three fingers; trigger/touch
controls index and thumb. A full fist uses grip and trigger with thumb resting
on the controller. These empty poses never replace accepted held knife/firearm
poses, and empty-hand fire remains blocked.

MovementDirection=head (default) or controller selects horizontal headset/left
controller aim heading for locomotion. It does not change weapon aim or turning.
Absent/near-vertical controller aim falls back to head heading. The ninth VR
settings row changes the preference immediately and saves it to the active INI.
The owner-reported missing grenade guide is still open. Knife transfer is not
implemented. v27 has 55/55 CTest and 12/12 GPU passes; headset acceptance pending.

## v28: mirrored fists, independent free arm and traversal

The v27 empty-finger inward axis was reversed. Native saved wrist/palm anatomy
proves left flexion must move toward +controller-palm X and right toward -X.
Joint lengths and the accepted held right-hand/knife attachment stay unchanged.
The free left hand restores canonical reference bones 2..27 and palm-relative
finger anatomy before its IK solve, including during right-hand firearm use.
Its clavicle follows its own shoulder. Supported rifle/pistol poses remain.

TraversalControls/TraversalView and NativeComfort add the exact local ladder
container and stock parachute profile. Camera comfort preserves entry heading
and follows real mount translation. Native camera look/bank/bob is excluded;
physical HMD movement still applies. The moving ladder container is distinct
from its static ladder geometry. Native +0x19f6b0 updates container translation
and calls its matrix setter at +0x19fa70. No physics/position writes were added.

Sustained native falling velocity triggers a release/press cycle on the native
Space jump/chute action, respecting the game's deployment rules. Grip/pull
strokes drive native ladder W/S with feedback-limited distance debt. Release
stops the gesture; left-stick up/down is the fallback. Mounted state suppresses
infantry holstering, ADS and stance input, while preserving the held-state latch.
This is gesture-controlled native climbing, not rung-snapping hand animation.

The optional lobby uses an owned walker pack produced by ExportLobbyScene.py,
including articulated child transforms and diffuse textures. The GPU room adds
a lit hangar and a depth-tested, mipmapped MSAA mesh pass with all state restored.
No extracted meshes/textures are committed to public source. Missing/invalid
pack retains the previous room. Loader corruption guards are deterministic tests.

Win32/x64 built; 57/57 CTest, 13/13 GPU fixtures passed. The lobby image was
reviewed and measured ~3.2 ms per 1280x720 stereo pair including readback with
hardware vertex processing. Headset/gameplay acceptance remains pending.

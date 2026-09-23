# BF2142 VR development port

This branch adapts BFVR v1.0.2 to BF2142's native D3D9 renderer. It is an
experimental development build, not a finished VR release. BF1942 targets remain
available; their game addresses and D3D8 translator are not used inside BF2142.

## Current combined build

The current v18 development client implements the following base features;
version-specific additions and verification limits are recorded below:

- Separate runtime-FOV eye rendering, 6DoF head poses, an upright neutral pose,
  recentering, and configurable physical translation scale/height offset.
- Native HUD and Flash menu isolation into a transparent target. Multiple UI
  batches accumulate within an eye. The target matches the game's antialiasing,
  including 4x MSAA, and resolves before desktop composition.
- Suppression of native internal Present calls during both eye renders. Eye
  readback precedes DISCARD; the completed pair presents once. Camera state is
  restored after each eye and the second render receives zero time delta.
- Local-player gating: the login screen's placeholder RenderView never becomes
  an eye image. Login/loading screens use a separate UI panel over empty eyes.
- Persistent local-player gating for extra desktop Presents. A missing stereo
  pair or transient camera mode is never interpreted as a menu while a valid
  local player exists; this prevents full-scene UI publication and repeated
  recentering between world frames.
- A right-hand tracked menu ray, per-eye beam, and high-contrast UI cursor.
  Source aspect-fit padding is excluded from ray hits. A single explicit anchor
  is shared with the presenter; panel width/distance remain 1.6/1.5 metres.
  Native frontend state plus cursor visibility selects menu controls inside a
  match. A trigger held
  while opening a menu must be released before clicking.
- Controller sticks and buttons through game-local DirectInput8 state/buffer
  overlays. Physical keyboard/mouse input is retained. Lost focus or a sample
  older than 150 ms releases synthetic buttons. Laser targeting moves the OS
  cursor only inside the foreground game window, with native window-message clicks at
  that same location. No global button/key events are generated.
- An experimental native controller-hand adapter with wrist tracking, elbow
  IK, preserved finger poses, animated weapon attachments and off-hand support.
  The main native projectile matrix is mapped into the solved weapon frame for
  the verified local firearm. v11 hands were played in a headset; v12 alignment
  and ergonomics remain pending.
  The older visual-only TrackedWeapon option remains off and is mutually
  exclusive with MotionHands.

Native entry points require matching code signatures and vtable relationships.
Unrecognized builds fall back to ordinary rendering. Game simulation remains
outside the eye pair. Native D3D9 state-cache ownership is preserved; desktop
composition restores the target, depth surface, viewport and render states.

## Launch and settings

Keep BF2142VRLauncher.exe and BF2142VRClient.dll together outside the game folder.
The x64 presenter needs its matching runtime/openxr/win64/openxr_loader.dll and
assets directories. Connect the headset/OpenXR runtime before a VR launch.

```powershell
.\BF2142VRLauncher.exe --game-dir 'D:\Games\Battlefield 2142' --windowed --presenter 'D:\BFVR-x64\BFVRPresenter.exe'
```

`--inspect` checks installation/images without launching. Omitting `--presenter`
keeps desktop forwarding mode. `--windowed` selects 1280x720. `--mod FOLDER`
selects an already-installed mod under the game's mods directory; Remaster is
not installed or patched by this launcher. Exit an existing game normally
before launching another session.

The client reads BF2142VR.ini beside the launcher, or the absolute path in
BF2142VR_CONFIG. See the example under src/bf2142. Settings take effect next
launch. WorldScale converts physical meters to game units; HeightOffset adds a
small vertical adjustment to the authored camera. Home or right-stick click
captures a new neutral pose, including in menus. Tracking/focus loss lasting
at least 750 ms triggers one reset on return. The first playable world also
captures neutral.

| Quest control | Game action |
| --- | --- |
| Left stick | Move relative to head direction |
| Right stick | Turn (vertical look disabled in MotionHands mode) |
| Point right controller | Menu laser/cursor |
| Right trigger | Fire / menu click |
| Right grip | Native ADS action (VR scope rendering still unfinished) |
| Left grip near the foregrip | Support the weapon with the left hand |
| Left trigger + left grip | Crouch |
| A | Jump; click laser target in menus |
| X | Reload |
| Y | Interact/use |
| B | Escape/back |
| Left-stick click | Sprint |
| Right-stick click / Home | Recenter head pose |
| Hold left trigger + X / Y | Next / previous weapon |
| Left menu button | Scoreboard, when the runtime exposes that button |

Mappings assume the game's default keyboard/mouse controls. Runtime-reserved
buttons remain under runtime control. In-match deployment/vehicle behavior and
hardware controller operation still need testing. v11 hands have been played in a headset; the v12 corrections still need a
combined match check. Manual magazine handling, tool-specific hand poses and
vehicle-specific controller mappings are not implemented. MotionHands=0 restores
the previous weapon behavior and controller mappings.

The BF2142 producer sets two additive protocol-v23 flags. FullEyeTextureFov
fills world eye destinations without aspect-fit bars; UI and BF1942 retain
aspect fit. OwnControllerMappings disables the BF1942 presenter's A-hold quick
menu, B-hold recenter and map shortcuts so they cannot conflict with BF2142
controls. Use the matching updated presenter; the shared record layout has not
changed.

## Local diagnostics and verification

`--diagnostic-stereo` replaces the headset with bounded synthetic tracking and
saves local eye/UI images beside the launch log. It connects the input API but
never publishes controller commands. It cannot be combined with --presenter.
Normal player launches do not save image diagnostics.

The user previously reached a headset match and confirmed that v4 removed the
black bars, but reported remaining duplicate images and broken menus. v4's
presumed interface boundary was actually a first-person weapon pass; v8 uses
the recovered HudManager and Flash display boundaries instead.

Verification for the current pass:

- Full Win32 build and 40/40 deterministic CTests passed; matching x64 presenter
  built. Tests cover pose/depth math, controller freshness, input release,
  buffered peeks/full buffers, physical input preservation and argument quoting.
- BF2142VRRendererSmoke passed on the local D3D9 GPU, including 4x MSAA UI
  accumulation, alpha, depth/viewport/target restoration and Reset.
- BF2142VRStereoIntegrationSmoke passed with and without 4x MSAA. It drives the
  actual stereo session with a known GPU scene: distinct eye images, synthetic
  head motion, HUD absent from world images, 120 internal render/Present calls,
  60 time advances, 60 pair Presents plus 60 extra desktop Presents, and just
  one initial neutral capture. Native game entry points are
  replaced in this harness; it does not prove native game integration.
- A live v7 Verdun bot-match diagnostic rendered distinct eyes and survived the
  match transition. It exposed the game's 4x MSAA target rejecting the old UI
  capture. v8 adds matching multisampling and passes the corresponding GPU tests.
- The user's v8 report confirms that menu distance looks good and VR otherwise
  works well, but reports an obstructing flat scene and missing/ineffective menu
  cursor. Live v8 logs show successful UI isolation and repeated neutral captures.
- v9 pointer tests check aspect-fit hits/padding, tracked-pose requirements,
  held-trigger suppression, off-panel rejection, per-eye laser differences,
  cursor transparency and BGRA/RGBA conversion. New headset behavior and native
  menu click operation still require the next combined user run.

Run the GPU probes explicitly; they are not part of deterministic CTest:

```powershell
.\BF2142VRRendererSmoke.exe
.\BF2142VRStereoIntegrationSmoke.exe
.\BF2142VRStereoIntegrationSmoke.exe --msaa
```

CPU readback plus D3D11 upload remains provisional and costs performance. A
resolution change currently ends stereo and requires relaunch at the new size.
No final performance, multiplayer, vehicle or Remaster compatibility claim is
made. See BF2142_CAMERA.md for recovered interfaces. Continue substantial work
in batches; do not ask the player to repeat a flat preview or test every edit.
## v10 / v10a follow-up

The user reached a v9 bot match on attempt three and confirms the floating HUD
and menu placement look good. Earlier attempts crashed during D3DX effect
creation, before the first gameplay stereo pair. v10 caps loading/menu image
transfers at roughly 30 Hz and logs the failed shader HRESULT and memory
headroom if this happens again. The native crash is not yet proven fixed.

v10's independent recording window was rejected after the user reported mouse
interaction breaking across two desktop windows. v10a reverts that change and
restores the previous embedded, input-transparent desktop preview. There is one
game window to click. The client/HMD rendering and accepted menu placement stay
as before. The active recording preview can be closed via its own WM_CLOSE
handler without ending the headset session; a new launch uses the v10a presenter.
The user now confirms OBS recording works. Do not reintroduce a second automatic window.

## v11 user report and v12 follow-up

The user confirms v11 gameplay, controller hands and OBS recording work, while
reporting laser clicks, invisible pause UI, shot misalignment, hand jitter,
flat ADS and recentering issues. v12 contains the combined fixes:

- Laser clicks use the native Flash window-message route; B sends the native
  Escape character in menus. Held-entry suppression and 150 ms expiry remain.
- Explicit Flash menu batches continue headset frames/input even without world
  rendering. They never authorize copying the full scene onto the UI panel.
- Main projectile launch and muzzle velocity follow the tracked firearm instead
  of modifying only the secondary projectile matrix; preserve native local
  offsets/deviation and inherited player movement.
- Stable wrist-to-weapon bindings reduce animation-driven hand jitter.
- Right-stick/Home recenter works in menus; sustained tracking loss resets on
  return. Runtimes that retain focus/tracking when the headset is removed may
  still require manual recentering.
- Right grip lights a provisional gun-mounted red dot. It is non-magnifying and
  uses generic placement without depth occlusion; weapon-specific/magnified
  scopes and native ADS accuracy behavior remain unfinished. See
  [native integration notes](BF2142_HANDS.md).

v12 passes all 41 Win32 CTests, builds the matching x64 presenter and passes the
renderer/stereo GPU probes with and without 4x MSAA. The stereo fixture now
exercises 120 UI-only pause frames and resumes without recentering. Native
headset behavior still needs one combined run; keep v11 as a fallback.

OBS recording is confirmed by the user. The single game window and accepted
floating HUD/menu placement are preserved. Missing weapon backs/stocks are
explicitly deferred; bare-hand finger tracking is outside this controller pass.

## v13 follow-up

The v12 headset run confirms menus and gun-directed bullets work. The user
reported an incorrect EU assault rifle left grip, rejected the generic circle
optic and missing ADS action, and experienced a crash possibly while firing.
v13 preserves the working menu/shot paths, restores right-grip native ADS,
removes the prototype circle and settles the left binding from a stable authored
holding pose after draw. Proper VR scope rendering remains unfinished.

The saved dump identified a missing native renderer query record, distinct
from earlier shader-loading failures. A profile-checked guard uses the existing
ordinary draw path for missing records; valid records keep native handling.
Both builds, 41 CTests and renderer/stereo/MSAA checks pass. A separate mapped-
renderer probe verifies the real lookup/detour with valid and missing records
without launching the game. Live crash recurrence and hand placement still need
one combined headset run. The v12 launcher/configuration is preserved locally.


## v14 accepted; v15 deployment pointer fix

The user reports v14 feels very good overall after infantry level-heading and
head-recoil isolation. v15 adds native deployment/class-menu detection so laser
activation and controller menu routing do not depend solely on Windows cursor
visibility. Existing clicks, panel geometry, accepted hands/aim and OBS behavior
remain intact. The v14 launcher/configuration is preserved locally.

Both builds, 43 CTests and renderer/stereo/MSAA checks pass. The native profile
and current deployment state were verified read-only against the installed game.
Headset selection of class/spawn points in v15 still needs the next user run.

Next priorities are proper VR gun optics, reload/weapon-selection interaction,
vehicle controls and comfort, then performance/stability and Remaster support.


## v16 weapon optics

Right grip retains the game's ADS action. On EU/PAC assault rifles and the EU,
PAC and unlocked sniper rifles, align either eye behind the physical sight to
activate a magnified lens view. Assault optics start at 2x and sniper optics at
4x, following subsequent native zoom changes. The supported gun keeps its
normal model, and the surrounding headset view remains at its normal FOV.
The existing grip, aim, recoil-free head orientation and floating HUD remain.

An additional zero-delta world render runs only while an eye is aligned. The
scope texture never becomes the desktop frame; the normal eye and HUD are
restored for the existing single-window recording path. Both builds, 44 CTests,
and stereo/scope/MSAA checks pass. Live native/headset optics are unverified.
Lens compositing does not yet use depth; hands can overdraw incorrectly across
the sight. The extra scoped render has a frame-rate cost. Mesh replacements,
Remaster, vehicle sights and other weapons retain uncalibrated/native behavior.
Set WeaponOptics=0 under [VR] to use the prior ADS presentation on next launch.


## v17 deployment input, physical ADS and rifle mesh repair

The user accepts the v16 sight presentation. Deployment still had no laser,
right-grip ADS shifted the hands, and missing gun surfaces were visible in VR.
v17 addresses these together without changing the accepted comfort heading,
projectile direction, floating panel placement or single-window recording path.

The spawn/class screen uses native HUD widgets, separate from the Flash
frontend targeted in v15. A signature-checked adapter reads HUD mode and GUI
size and routes the existing ray into native absolute movement and select/up
queues. Inactive allocated Flash objects no longer block native HUD input;
active Flash screens keep their existing route. Held buttons are released on
lost focus/ray/input, and stereo replay does not enqueue a second click.

Align either eye behind a calibrated sight for 180 ms to engage native ADS;
leave the eye box for 300 ms to release it. Right grip remains an override.
Native cancellation blocks immediate reacquisition until the gun is lowered
and the grip released. AutomaticADS=0 restores grip-only use. WeaponOptics=0
restores the prior native sight presentation. Hand bones retain their settled
holding shape during supported ADS and a 350 ms exit transition; native weapon
parts, firing, controller wrist targets and the existing solver continue.

The local mesh repair covers eu_ar_rifle, as_ar_rifle, eu_sni, as_sni and
unl_adv_sni. Reversed solid faces reveal previously culled backs; the PAC rifle
also receives its missing stock from the existing third-person model. UVs and
textures are reused, so this is surface completion rather than new HD artwork.
The tool preserves transparent materials and every untouched LOD/ZIP member.
No owned game assets are included in source. The private installation retains
the original archive and a checked v16 fallback.

47 Win32 CTests, both architecture builds, five mesh tests and renderer/stereo/
scope/MSAA GPU checks pass. v17 has not had an actual headset playthrough;
physical alignment, native spawn selection and repaired textures need that
combined run. v16 optics and v14 comfort have positive user headset reports.
Remaining work includes lens depth occlusion, other weapon models, more native
VR reload/selection interaction, vehicles, performance and Remaster support.


## v18 visual regression correction

The v17 headset report shows a displaced support hand and bright blue surfaces
stretching from the gun. v17's mesh rewrite preserved only the first of eight
alpha-blended face-sort lists while leaving the header's count at eight. Later
view directions therefore consumed unrelated indices. v18 retains and bounds-
checks all lists; all 80 lists in the five guns' 10 transparent materials match
the original, including positions, UVs and shader attributes. Other LODs and
353 untouched archive entries match too. Opaque backfaces and PAC stock repair
remain. An independent binary fixture reproduces and rejects the old output.

The v17 whole-arm/finger ADS cache is disconnected, and NativeHands::Apply
matches accepted v16 logic. AutomaticADS defaults to 0, including the installed
configuration, to restore the accepted right-grip ADS behavior while preserving
v16 lens optics. AutomaticADS=1 remains experimental, not a verified fix.
The native deployment pointer code from v17 is retained, with no new headset
confirmation yet. V18's actual hand feel and textured appearance still need
one normal playthrough. Use the usual shortcut; v16 remains the fallback.


## v19 weapon asset coverage, using the v18 runtime

The user reported that unfinished gun surfaces remained. Read-only inspection
confirmed the live process loaded the v18 client and the installed archive was
the corrected v18 file. The active weapon was eu_mg, outside the five rifles in
the previous patch. Do not explain that report as an accidental v16 rollback.

The asset repair now covers 28 firearm/attachment models: EU/PAC rifles, MGs,
SMGs, handguns, launchers, unlock rifles/MGs/shotguns and underbarrel variants.
All 54 first-person normal/aiming mesh variants receive reversed solid faces.
Existing transparency and all 400 directional index lists are preserved; world
LODs and 330 unrelated ZIP entries remain identical. The PAC rifle's stock
addition remains. Nine regression tests pass, including machine-gun normal/ADS
coverage. Native v18 source hashes are unchanged, so the existing verified x86
client and x64 presenter are reused. The launcher selects the v19 asset archive.

This reveals existing textured faces from the reverse direction; it is not new
HD texture painting or a complete reconstruction of every deleted component.
The user has not yet confirmed these 28 models in a headset. If the game is
already running, stage the archive and apply on the next full launch; never
replace the mounted ZIP while a match is active.


## v20 completes genuinely absent surfaces

The user's screenshot and 69-second gun-rotation clip show the first-person
asset's absent sides, rear caps and underside. The installed v19 archive was
confirmed; broadening reversed-face coverage did not reconstruct those areas.
v20 therefore adds actual outer surfaces from each corresponding world model.

The normal FP model keeps all of its existing materials/triangles. For each
compatible world triangle, a part-specific surface query checks whether the FP
mesh already supplies a nearby outward-facing surface. Missing regions retain
the donor's actual UVs/textures. Adaptive subdivision limits overlap along
coverage boundaries, and a 0.5 mm inset gives the detailed native skin priority.
Voss variants receive their measured FP scale/offset; the PAC pistol bolt maps
to its corresponding FP part. Unknown donor parts are excluded. Third-person
LODs remain native, and v19's aiming-mesh backface handling remains.

The completed archive has new material surfaces on all 28 models. All prior
v19 material/vertex/index data are unchanged, along with 50 transparent
materials, 400 alpha-sort lists and 330 unrelated archive entries. Fifteen
mesh/completion tests pass. Textured before/after renders inspected both rear/
underside and normal-view sides for the EU rifle, EU MG, shotgun and PAC rifle.
The original high-detail side remains; the formerly omitted areas use the
world model's lower-detail texture where needed. These renders are offline
previews, not a headset test. Runtime stays at the accepted v18 recovery build;
v20 is a weapon-asset update applied after a full exit/relaunch.

## BF2142 v21: physical sight activation and centered deployment input

- Supported sights engage native ADS after either eye aligns for 180 ms and
  release after 300 ms out of alignment. Right grip no longer forces ADS in
  automatic mode; native reload/sprint cancellations require lowering the gun.
- Add the EU LMG's measured 1x reflex window. Keep its normal completed model
  during native ADS, and draw only a collimated dot inside the glass. It does
  not request another world render or magnify a patch around the weapon.
- Correct deployment-pointer coordinates to include the native HUD rectangle's
  left/top origin. The native canvas is centered; zero-origin input produced
  a half-canvas offset between the VR dot and the actual clickable point.
- Preserve accepted v20 weapon assets, v18 hands, comfort and OBS behavior.


The user has confirmed the v20 weapon geometry repair in-headset. V21 is a separate fix checkpoint before the larger interaction pass; headset validation remains pending.


## v22 desktop development and interactions

`--desktop-vr --windowed` starts the real x86 game/client without the x64
presenter or OpenXR. It uses the same camera, ADS, hand, inventory and UI hooks.
A one-eye composite is shown in the existing game window. This is not a second
interactive mirror and does not require SteamVR or Quest connectivity.

Desktop controls: F1 normal hold; F2 align the measured sight to the left eye;
F3 primary/back, F4 pistol/hip, F5 knife/chest, F6 utility/left belt,
F7 utility/rear belt, F8 grenade/front belt, keypad 0 extra utility/right rear belt; right Ctrl grabs, right Shift
supports, F9 triggers, F10 opens/closes the menu. Keypad 4/6, +/- and 8/2 trim
hand XYZ. PageUp/PageDown look up/down, End levels the head; Home recenters. Left Ctrl simulates
index touch, left Alt thumb touch, left Shift index squeeze. F11 simulates
tracking/focus loss. Mouse aims the simulated menu ray; F9 tests controller
clicks. Normal keyboard controls remain active. These extra keys are read only
in desktop mode and while the game owns foreground focus.

v21 is frozen as a separate ADS/menu-only checkpoint. v22 uses a separate
configuration with BodyInventory, FingerPoses and BodyEquipmentFile. Disable
individual features or use the checkpoint launchers for rollback. Owned weapon
archives are selected only at launch and never replaced in a running game.
Desktop tests do not establish headset comfort, touch delivery over Steam Link,
or multiplayer compatibility. Those still need one combined user playthrough.


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

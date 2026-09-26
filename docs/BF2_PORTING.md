# Porting this code to Battlefield 2

Start with the [IK transfer guide](ik/README.md) and [beta changes since alpha](BF2_POST_ALPHA_UPDATES.md) for current multiplayer work.

BF2142 VR already supplies the VR runtime and interaction systems. A BF2 port should connect those systems to BF2's native renderer and gameplay objects. No BF2 runtime profile has been implemented or validated in this snapshot, so there is no supported BF2 launch command yet.

## Start here

Build and run the existing deterministic tests without a headset. Use the current source as a reference and add a BF2-specific adapter/target, preserving the working BF2142 profile. Read src/bf2142/CMakeLists.txt, BF2142VRLauncher.cpp, BF2142VRClient.cpp, StereoSession.cpp and NativeStereo.cpp to follow startup through eye rendering.

## What carries over

| Component | Source | Porting work |
| --- | --- | --- |
| OpenXR sessions, poses and controller actions | src/openxr | Reuse runtime infrastructure; retain matching presenter/protocol versions. |
| x64 D3D11 presentation and x86-to-x64 transport | src/presenter | Reuse shared texture/control transport; audit game flags and mirror behavior. |
| Eye projection, tracking and UI ray math | src/stereo; bf2142/TrackingMath, StereoCamera, MenuPointer | Reuse math; establish BF2 coordinate conventions and panel anchors. |
| Stance, turning, locomotion, inventory and grip policies | bf2142/ComfortControls, ControllerPolicy, BodyInventory, WeaponGrip | Reuse policies/tests; map BF2 actions, weapon IDs and ownership. |
| Arm solving and controller touch poses | bf2142/HandPoseMath, ControllerHandPose, FingerPose | Reuse solvers; replace bone topology, bind transforms and finger axes. |
| Physical ADS and vehicle interaction policies | bf2142/AutoAdsPolicy, WeaponOptic, VehicleControls | Reuse policy concepts/math; recover BF2 optics, seats and native aim input. |
| Weapon surface repair / local asset export | scripts/bf2142 | Reuse parser and reconstruction approach after checking BF2 mesh variants; create BF2 donor/material profiles. |

Shared code still contains game-specific assumptions and flags. This is an adaptation guide, not a claim that every listed file can be copied unchanged.

## Native boundaries to recover for BF2

- Launcher process identity, module discovery and supported version detection.
- Renderer/camera entry points, frame boundaries, native Present calls, UI isolation and state-cache ownership: NativeStereo, NativeUiCapture and NativeMenus.
- Local-player ownership, object ancestry, offsets and vtable signatures: NativeHands, NativeComfort, NativeVehicle and NativeQueryGuard.
- In-match frontend/deployment state and cursor mapping: NativeHudState, NativeMenuState and NativeHudPointer. Ray, UI target, viewport and mouse coordinates must share one transform.
- Bone indices/topology, bind transforms and animation state. BF2142's verified 70-bone rig is not a BF2 guarantee.
- Projectile/fire matrices, weapon ownership, animation transitions and actual simulation aim. A visually aligned gun is insufficient if launch/hit registration uses another transform.
- ADS/HUD/crosshair state and scope surfaces: NativeOptics, NativeCrosshair and GunOptics.
- Vehicle seat tables, turret parents and angular limits. Fixed guns retain vehicle aim; articulated guns consume the game's supported aim inputs.

Use byte signatures plus structural checks for each recovered native profile. Unknown binaries should fail closed instead of accepting BF2142 offsets because the games share an engine family. Do not apply LegacyShaderMemory blindly: it protects a particular verified d3dx9_29 compiler path and low-address allocation requirement.

## Avoid repeating the difficult rendering bugs

One game simulation step must produce both eyes; restore native camera/render state afterward. A temporary loss of stereo data must not publish the whole world as a flat menu panel. Keep rendering head orientation separate from gameplay recoil, ladders and parachute animation. Snap turns must rotate the hand/body basis coherently, including the HUD/minimap basis.

The missing gun sides were missing first-person mesh surfaces, not only backface culling. The repair pipeline uses each player's own donor geometry and materials while preserving supported first-person animation and transparent optic/effect passes. Blanket two-sided rendering exposed unwanted interiors and effects. BF2 needs its own donor/LOD/material mapping and tests; do not distribute either game's meshes or textures.

## Suggested implementation order

1. Add BF2 installation/version inspection and read-only native discovery using the existing launcher patterns.
2. Connect native stereo, UI capture and frame/state restoration; use synthetic pose/GPU fixtures during development.
3. Connect local ownership, hands, firearm aim and menus. Reuse interaction policies and preserve default keyboard/mouse behavior.
4. Add BF2 weapon repair profiles, optics and vehicle seats; batch headset checks around complete playable changes.
5. Test multiplayer connection, authoritative aim/hit registration, respawn and vehicles separately from remote IK replication.

Desktop simulation and deterministic tests help iteration without repeatedly reconnecting a headset. Final stereo comfort, physical alignment and real runtime behavior still need a headset. The released alpha has local-only arm IK. Subsequent private multiplayer development includes a pose relay and remote IK; see [post-alpha updates](BF2_POST_ALPHA_UPDATES.md) before duplicating that work. Internet transport and two-VR-user acceptance remain pending.

# IK transfer guide: BF2142 VR to BF2

Entry point for the observer IK shipped in **0.2.0-beta.1**. Source remains in its tested modules; this guide separates reusable policies from BF2142 engine integration. Read [architecture](ARCHITECTURE.md), [lessons learned](LESSONS.md) and [porting checks](PORTING.md) before copying hooks.

## Source map

These are reusable starting points, not a standalone engine-independent SDK.

| Component | Source | Port boundary |
| --- | --- | --- |
| Rigid poses, sessions, flags, curls | [PoseProtocol](../../src/bf2142/multiplayer/PoseProtocol.h) | Both ends must agree on layout, units and frames. |
| Smoothing and torso follow | [RemotePresentation](../../src/bf2142/multiplayer/RemotePresentation.cpp) | Reuse smoothing; replace torso topology and neutral calibration. |
| Head, arms, palm binding, fingers | [RemoteArmMath](../../src/bf2142/multiplayer/RemoteArmMath.cpp) | Reuse rigid math and solve policy; replace 80-bone mapping, palm/finger axes and reference capture. |
| Cosmetic self-collision | [RemoteCollision](../../src/bf2142/multiplayer/RemoteCollision.cpp) | Reuse capsule/elbow-circle approach; adapt body landmarks. |
| Social contact haptics | [FistBump](../../src/bf2142/multiplayer/FistBump.cpp) | Reuse freshness, approach, swept contact and cooldown; verify scale/knuckle conventions. |
| Native animation lifetime | [NativeNetwork](../../src/bf2142/multiplayer/NativeNetwork.cpp) | Rewrite and signature-verify native reads/writes, lifetime and finalize callbacks. |
| Item mesh/attachment visibility | [NativeRemoteWeapon](../../src/bf2142/multiplayer/NativeRemoteWeapon.cpp) | Game-specific draw callbacks, ownership, item names and attachments. |
| Authoritative aiming/roster | [NativeServer](../../src/bf2142/multiplayer/NativeServer.cpp), [NativeRoster](../../src/bf2142/multiplayer/NativeRoster.h) | Independently verify hooks and player identity. |
| Internet bridge/downloads | [community](../../src/community), [hosting](../BF2142_COMMUNITY_HOSTING.md) | Share TLS/UDP/package infrastructure; adapt game admission. |
| Regression examples | [NetworkTests](../../src/bf2142/multiplayer/NetworkTests.cpp), [RemotePresentationTests](../../src/bf2142/multiplayer/RemotePresentationTests.cpp) | Synthetic fixtures can travel; BF2142 indices are not BF2 assertions. |
| Haptic dispatch | [OpenXRHaptics](../../src/openxr/OpenXRHaptics.h), [shared IPC](../../src/presenter/SharedPresentationProtocol.h), [EquipmentHaptics](../../src/bf2142/EquipmentHaptics.h) | Match client/presenter versions; controller paths are per hand. |

Versions: **pose v4 (616 bytes), voice v1, shared client/presenter IPC 26**. Shared IPC is local to a PC; the internet bridge has its own authenticated framing. Current cloud bridge label is `v35-community`. A presenter IPC change does not itself require replacing the cloud service.

Recorded VR-to-flat sessions show improvements in head rotation, knife/rifle visibility, arm smoothing and body yaw. Latest arm-base recoil suppression, segment clearance and equipment haptics have deterministic tests; final visual/haptic acceptance is pending. Two independent VR users' fist-bump feel needs confirmation. Collision is cosmetic, not a physical avatar or hitbox.

Project code is MIT with attribution; third-party components retain their licenses. Do not redistribute extracted soldier meshes/skeletons, weapon archives, private recordings or memory dumps. Create game-derived fixtures from your own installation.

# Changes since the public alpha

**0.2.0-beta.1** source/downloads supersede the public v30 / 0.1.0-alpha.1 baseline. This is no longer a private-only feature list. The dedicated [IK guide](ik/README.md) is the main entry point for BF2.

## Shipped in beta

- Dedicated-server controller aiming, ownership-aware pose relay and native join proof.
- Real remote head/arm/item IK, empty hands/holsters, knife/rifle visibility and approximate finger curls.
- Coherent observer smoothing, connected torso follow, calibrated neutral bone frames and elbow continuity.
- Cosmetic body/hand/arm clearance and fist-bump detection/haptics.
- Observer recoil isolation and stronger equipment cues; latest visual/haptic acceptance pending.
- Flat addon and isolated second-account observer mode. Public flat addon serves ordinary players; isolated observer profiles are a developer option.
- Multiplayer ADS, camera/LOD flicker, invisible hands, holster visibility, heading and snap-turn corrections.
- Left-hand support-crate pickup/throw/cooldown and previous-weapon restoration. Re-selecting an active slot no longer toggles fire mode.
- Titan/silo world-marker suppression, headset-off desktop/login work and stance recenter recalibration.
- Shoulder radio, Opus proximity voice and configurable enemy/range policy. Voice worked with the second vrtester client; broader device/network testing remains.
- Signed downloads, offline flat join EXE, optional URI handler, TLS/encrypted UDP bridge and Windows host/map-management scripts.
- Outside-network flat/VR gameplay exercised. Co-op bots/map voting configured on the temporary test server.

## Code map

[IK modules](ik/README.md), [multiplayer](LOCAL_MULTIPLAYER.md), [voice](BF2142_VOICE.md), [Hub integration](FLAT_ADDON.md), [hosting](BF2142_COMMUNITY_HOSTING.md).

Native pose v4 / 616 bytes, voice v1, local presenter IPC **26**. Keep native/presenter builds matched. Adapt BF2 native hooks, player identity, skeleton topology, weapons and executable signatures; do not copy BF2142 offsets as universal engine facts.

## Separate experiments — not in this beta

Branch `dev/singleplayer-experiments` contains EU rifle native-blue-sight and optional rifle/P33 reload experiments, chest ammo pickup, magazine insertion and revolver cylinder closure. Rifle optic interior work is there too. These are parked separately; coordinate before rebuilding them. Broader reload support and magazine grip polish remain future work.

## Next

Two-VR fist feedback, broader voice routing, body/hand polish, roomscale collision testing, Reclamation integration, optional reloads and physical vehicle controls. Beta is a useful baseline, not universal headset/rig acceptance.

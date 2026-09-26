# 0.2.0-beta.1 validation and limits

The owner authorized publishing without another headset test. Earlier development builds were exercised in multiplayer, including an outside-network flat player and a same-PC second account observing VR motion. Recordings showed accepted head/hand visibility, smoothing, knife handling and body yaw following. The owner also confirmed proximity voice worked with the second vrtester client.

This candidate builds for x86 and x64 and passes 71 native CTests. Community tests pass 279 checks, including signed package verification and a live local TLS/encrypted-UDP pose/audio exchange. Release CHECKS.txt records installer/payload verification. Synthetic tests do not establish headset acceptance.

Latest fixes awaiting visual/haptic confirmation: observer-only arm/collar stabilization against native recoil, arm-segment clearance, and stronger equipment haptics including left-hand crates. The previous torso calibration is retained; not all abdomen/shoulder deformation is solved.

## Known limits

- Residual body/shoulder/finger jank with extreme reaches and differing classes/stances. Cosmetic capsule collision does not prevent every gun/body/world intersection or change hitboxes.
- Fist-bump haptics have deterministic contact tests. Two independent VR players still need to confirm bilateral feel. Flat players observe gestures but do not supply tracked fists.
- Proximity voice worked in the two-client test. Wider separate-PC microphone routing, squad/commander service configuration and latency still need verification; an earlier outside-player report had no audible proximity voice.
- Primarily Quest 3/Steam Link/SteamVR testing. Headset disconnect can need reconnect/recenter. Controller inputs give approximate curls, not optical finger tracking.
- Roomscale walking/body collision without stick input remains a future test. Physical crouch/prone alone does not prove collision-aware roomscale locomotion.
- Stock v1.51 required. No supported Remaster/other weapon-pack setup.
- Bots depend on map navigation. 64 slots and 62 stock bots do not prove 64-human performance; some modes/maps lack bots.
- Main beta excludes the parked singleplayer manual-reload/blue-sight experiments, physical steering wheels, knife hand-to-hand tossing and rebuilt cockpits.
- The test server is temporary. The mod remains usable for singleplayer; other community hosts need the server adapter, bridge and trusted descriptor setup.

# IK lessons learned

1. **A good first frame can hide feedback.** Native LOD may update only part of a skeleton. Restore all previous IK writes before native animation, not only arms after rendering. Otherwise last frame's chest/head/gear becomes next frame's input and deforms cumulatively.
2. **Smooth the attachment, not only wrists.** The item must retain its relative hand transform. Reset at respawn, slot/session reuse, recenter, stale/invalid packets and large discontinuities.
3. **Identity axes are not necessarily a neutral human.** BF2142's waist/spine/chest/neck bases have authored nonzero pitches. Forcing them upright can push out the abdomen. Verify against the owned third-person inverse-bind mesh, not first-person geometry alone. Stored skeleton rotations required inversion before composition for this profile. Residual posture variation remains.
4. **Rotate connected subtrees.** Different weights about one hip pivot detach shoulders/stretch skin. Rotate at each joint's attachment and propagate its full subtree. Preserve parent-local translations and native hips/legs.
5. **Native weapon animation is not controller intent.** Pistol recoil can change shoulder/elbow orientation even with correct wrists. Rebase validated arm/collar reference geometry to the live chest before solving. A fixed packet against different native recoil poses should yield the same observer arms.
6. **Distant rigs may collapse a chain.** Keep a validated detailed reference and rebase it into the current body. Do not treat zero-length animation LOD as anatomy or replace an entire remote soldier with the local rig.
7. **An outside elbow can still route the arm through the torso.** Check segment interiors and rotate on the two-bone circle to preserve lengths and wrist targets. Approximate capsule contact is not full mesh/world collision.
8. **Weapon visibility has multiple paths.** Empty hands may leave a native attachment or shadow rifle. Suppress only the verified owned item and restore scoped draw state; broad suppression breaks unrelated objects.
9. **Finger pose is inferred.** Capacitive controller inputs yield approximate curls. BF2142 has grouped finger chains; do not promise precise finger-by-finger tracking.
10. **Haptic events need independent channels.** Fist bumps must not consume equipment/menu cues. Use per-hand counters and matched IPC. A faint 15 ms menu cue can be imperceptible at a back slot; equipment needs its own policy.
11. **A mirrored bot only tests rendering.** It does not prove remote ownership or item state. Use a second actual client/account; receive-only mic avoids duplicate capture on one PC. Bilateral social feedback still requires two humans/headsets.
12. **Review ordinary motion separately from tracking loss.** Steam Link disconnect, headset removal and recenter are useful reset tests but should not be mistaken for normal motion quality.

Public tests use synthetic matrices. Optional replay fixtures from owned game data stay private. See [porting checks](PORTING.md).

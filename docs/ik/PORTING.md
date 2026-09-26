# BF2 implementation checklist

Keep BF2142 working while adding a separate BF2 profile.

1. Build math/tests and presenter without game files using [build instructions](../BUILD_BF2142.md).
2. Establish BF2 identity, third-person skeleton, units, matrix convention, palm frames and animation finalization. Verify every native hook through signatures and guarded layouts.
3. Map pelvis/spine/chest/head/face, collar/arm/elbow/wrist/finger groups, item bones and gear. Inspect teams/classes and animation LODs.
4. Verify neutral skinning against third-person inverse-bind data. Neutral output should retain authored shape; subtrees must preserve attachment translations. Use independent fixtures rather than duplicating implementation constants.
5. Port head/arms, then held-item orientation/visibility and fingers. Add body follow with hierarchy checks. Test fixed controller targets under recoil, reload and knife animation.
6. Check restore-before-finalize, entity deletion/reuse, respawn and partial LOD updates. Fall back on failed reads, nonfinite bases, implausible lengths or unknown rigs.
7. Reuse filtering, elbow continuity, contact and fist reset policies. Test crossed arms, two-hand support, impossible reach and exact palm/item attachment.
8. Adapt server admission, roster and controller firing separately. Keep native sockets loopback-only and public traffic on the authenticated bridge. Packet player IDs are not ownership proof by themselves.
9. Compare a real VR sender and flat observer: idle, every weapon, empty fists, support hand, holster, aim/fire, stance, turn/sprint, traversal/vehicles, death/reconnect and slot reuse. Same-PC second account is useful; add outside-network and two-headset checks.
10. Record limits, version protocols, package matching runtimes and retain rollback. Ship code/scripts only; generate owned assets locally.

Reuse MIT code with BFVR/BF2142 attribution and third-party notices. Future common-code refactors should expose a rig/identity adapter, not share unconditional BF2142 memory addresses.

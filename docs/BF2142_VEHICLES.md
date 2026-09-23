# BF2142 VR vehicle and parachute integration

v30 private build; local live interaction/headset acceptance pending.

## Player controls

- Left stick: native vehicle movement/steering, independent of the infantry
  head/controller movement-direction setting. Right stick retains native axes.
- Right trigger fires. Right grip is native alternate fire.
- Hold left trigger and tap X for driver/F1. Hold left trigger and tap Y to
  cycle seats 2-8. Occupied/absent seats are skipped by another press, not by
  assuming local occupancy. Y by itself still enters/exits a vehicle.
- APC: choose a passenger/pod seat, then right grip launches its pod.
- Titan: Y enters a pod launcher, right trigger launches. Right grip also maps
  the launcher's native alternate firing input.
- Aircraft pilot guns remain fixed to the aircraft. Pilot head look is free
  viewing, not flight steering. The Nekomata main cannon gets head pitch while
  its hull supplies yaw. Native turret traverse limits and projectile behavior
  are preserved. This is not yet physical cockpit controls.

## Native evidence and boundaries

The stock vehicle capability table comes from the owned Vehicles_server.zip
PlayerControlObject definitions and their child chains, stopping at other PCO
seats. Names/capabilities only are checked into source, not extracted assets.
PCO constructor RVA 0x1cd180 installs vtable 0x56de60. Template constructor
0x1cd9e0 installs 0x56e058. Base world getter is 0x2f6e20. The walker gunner
parent path includes vtable 0x5626d8 with rotational getter 0x1636f0 before the
chassis; its first twelve instruction bytes are separately checked. Live
read-only inspection confirmed buggy, walker gunner and static AV chains.

The alive local player owns weak soldier +0xcc and controlled object +0x80.
Instance +0x24 refers to the stock definition; validated small-string name is
at +0x10, length +0x20, capacity +0x24. Parent +0x34 walks to chassis; world
matrix is +0xa0. Cap traversal at 24 ancestors and reject cycles, dead objects,
unknown names/vtables and invalid transforms. Do not call native lazy matrix
getters from another thread. The current frame's cached camera at +0x40 is
read through the exact renderer/camera profile and must be near the seat.

VehicleView captures an entry offset relative to the chassis. Turret camera
yaw/pitch cannot feed back into the VR world pose. HMD orientation builds a
world target, compared with native camera direction; bounded relative mouse
input lets native turret physics converge. Fresh/focused input and exact
owner/seat/root identities guard cached camera state. Seat chords are edges,
primed across focus gaps/owner changes. Infantry locomotion rotation, stance,
holstering and automatic ADS are disabled while in these seats.

Parachute hand eligibility requires the existing exact stock Parachute profile
and local soldier identity. Preserve the native 70-bone topology; only bones
2-69 may change. Tracked hands use the existing level traversal camera in
skeleton space. Keep the weapon hidden for the glide and preserve its settled
bindings for landing. Fire pose publishing stays off while under canopy.

## Verification

60/60 CTest and 13/13 rendering fixtures pass; Win32 client and x64 presenter
builds pass. NativeVehicleTests covers local identity, stock names, malformed
names, root cycles, articulated ancestry/signature failure, death and bad
transforms. VehicleControlsTests covers chassis/turret separation, bounded
feedback, pilot exceptions, focus and seat edges. NativeMotionTests covers
parachute-only hand eligibility. These do not prove feel or correct aim in all
live vehicles; one combined in-game acceptance session remains.

Keep v29's shader allocation fix and the completed weapon/lobby assets intact.
No changes to network IK, vehicle meshes, physics or the recording setup.

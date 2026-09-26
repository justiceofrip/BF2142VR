# Observer IK architecture

## Data flow

1. The VR client constructs head, palm and held-item poses with validity/equipment flags. Controller input supplies approximate curls, not optical finger tracking.
2. The server adapter verifies native player ownership, relays fresh poses and applies supported controller aim to authoritative firing. Visual smoothing never feeds back into aim/damage.
3. A receiving VR or flat-addon client associates the player/session with a native soldier and verifies the rig. Both downloads contain the same receiver.
4. Rigid targets are filtered coherently. The item follows its filtered holding hand with a preserved relative attachment; filtering the gun independently creates drift.
5. Before native animation/finalization, restore exact prior IK writes. Then obtain a clean baseline, apply connected torso/head work and solve arms toward palms. Native pelvis, legs and locomotion remain authoritative.
6. Preserve segment lengths, choose a continuous elbow bend and test approximate torso clearance. Finger groups and item bones follow their wrist. Verified draw hooks prevent a holstered/left-held item leaving a ghost rifle.
7. Reset presentation state at identity/tracking/timing discontinuities. Unsupported layouts fall back instead of writing arbitrary memory.

## Frames and rig

Matrices use the existing row-vector composition: `local * parent = composed`. Body placement and soldier-local head/palms must not be mixed. `InverseAnimatedBone` accepts small native scale/shear but computes the actual inverse; transposing such a basis introduces attachment errors.

Palm-to-wrist calibration comes from target-rig geometry. Keep controller intent independent of native aim/recoil. Beta can rebase validated reference collars/arm chains into the current chest before solving; native pistol recoil then does not move observer arms whose targets stayed fixed. Per-frame wrist-local finger/item offsets remain attached.

BF2142 uses an 80-bone rig. Upper-body restore spans 11..74; native legs/unrelated attachments remain unchanged. These numbers, neutral spine pitches, palm topology and callback addresses are **not portable BF2 constants**.

## Contact and feedback

Hand separation is relaxed for deliberate two-hand grips. A torso capsule uses verified landmarks. Elbow clearance samples upper-arm/forearm interiors, then rotates on the analytic elbow circle. If bounded search cannot clear every segment it chooses improved clearance; it cannot guarantee zero intersections.

Fist contact requires fresh authenticated poses, closed empty hands, approach, swept separation and cooldown, with reset on jumps. It produces a local per-hand pulse, not player physics or a gameplay hit. A flat client can observe but supplies no tracked fist.

Equipment uses separate counters and stronger pulses than menu hover. Slow reminders while inside an available grab zone make back/belt slots easier to locate. Invalid tracking/focus prevents feedback; software cannot restore a controller the runtime stopped tracking.

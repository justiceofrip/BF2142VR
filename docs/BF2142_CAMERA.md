# BF2142 native camera integration findings

These are reverse-engineering observations from the installed 1.51 renderer.
The current development path uses signature-checked versions of these interfaces.
The user has confirmed v13 gameplay, menus, gun aim and improved hand placement.
v14 adds infantry camera comfort; its headset feel is still unverified. No proprietary game
binaries, shader source, live memory dumps, or personal logs belong in this
repository. Local inspection scripts and evidence remain outside it.

## Renderer identification

The OpenSpy installation loads a proxy as RendDX9.dll and the original renderer
as RendDX9_ori.dll. Camera functions below belong to the original renderer.
Resolve by module base and verified signatures/relationships; never apply an
address to the proxy or assume the DLL loads at its preferred base.

The native class registration string `dice.hfe.Rend.RenderView` references
factory RVA 0x66642. That factory allocates 0x434 bytes and calls constructor
RVA 0x671D0. The constructor assigns vtable RVA 0x1C6610.

| RenderView operation | Vtable slot | Function RVA |
| --- | ---: | ---: |
| Set viewport rectangle | 4 | 0x67070 |
| Set vertical FOV | 8 | 0x66B30 |
| Get vertical FOV | 9 | 0x66C10 |
| Set near distance | 13 | 0x66C30 |
| Get near distance | 14 | 0x66C60 |
| Set far-distance delta | 15 | 0x66C70 |
| Get far-distance delta | 16 | 0x66CA0 |
| Set height/width aspect | 17 | 0x66CB0 |
| Get height/width aspect | 18 | 0x66CE0 |
| Set projection mode | 19 | 0x66CF0 |
| Set camera-to-world transform | 25 | 0x67090 |
| Get camera-to-world transform | 26 | 0x66DD0 |
| Get derived view transform | 27 | 0x66DE0 |
| Get projection | 28 | 0x670C0 |
| Get frustum | 30 | 0x66E90 |
| Get combined view/projection | 34 | 0x66E50 |

The setter at RVA 0x67090 copies 64 bytes into object+0x40 through the native
matrix-copy helper, sets transform flags at +0x424 to 3, and marks +0x42C and
+0x42D dirty. Its profiled bytes are:

```text
55 8B EC 8B 45 08 56 8B F1 50 8D 4E 40 E8 ?? ?? ?? ??
B0 01 C7 86 24 04 00 00 03 00 00 00 88 86 2C 04 00 00
88 86 2D 04 00 00 5E 5D C2 04 00
```

This supplies a signature candidate, not sufficient authorization to install a
hook by itself. Confirm the enclosing executable section, unique match,
vtable getter/setter relationships, and native object identity together.

## Camera state and projection convention

| Object offset | Observed meaning |
| --- | --- |
| +0x18 | Projection mode; observed 0 perspective, 1 orthographic |
| +0x24 | Vertical FOV in radians |
| +0x2C | Near plane |
| +0x30 | Far-plane distance minus near plane |
| +0x34 | Height divided by width (0.5625 at 1280x720) |
| +0x40 | Camera-to-world 4x4 matrix |
| +0x80 | Derived view matrix |
| +0x100 | Combined view/projection |
| +0x140 | Projection matrix |
| +0x424 | Transform cache flags |
| +0x428 | Projection cache flags |
| +0x42C, +0x42D | Combined matrix and frustum dirty flags |

Projection builder RVA 0x66F10 computes yScale=cot(fov/2),
xScale=yScale*height/width, zScale=(near+farDelta)/farDelta,
zTranslation=-near*zScale, and row-vector m[2][3]=1. The near/far interval for
BFVR's existing projection math must therefore be (near, near+farDelta).
Do not treat +0x30 as the absolute far plane.

The matrices use the same row-vector, left-handed perspective convention as
BFVR's existing StereoMath helpers. Their D3D8 names do not imply a different
D3D9 matrix convention. Existing ComposeRuntimeHeadWithD3D8Camera and asymmetric
projection helpers are reuse candidates; retain finite/rigid-pose validation.

## Live ownership evidence

Read-only inspection of the running match found perspective RenderViews with
matching world position/orientation and 16:9 aspect, but different near/far/FOV
values. A renderer object with vtable RVA 0x1C1868 owns:

- World RenderView at renderer+0xF8, returned by slot 21 / RVA 0xD0C0.
- First-person RenderView at renderer+0xFC, returned by slot 55 / RVA 0xD0D0.

The world view had near=0.041 and farDelta=330. The apparent first-person view
had near=0.16 and farDelta=14.84. Their role assignments follow these getters
and matching poses; weapon behavior has not yet been tested with overrides.
An orthographic view was separate. Do not classify by FOV or depth thresholds
alone, and do not apply the headset transform to every RenderView instance.

Renderer global RVA 0x1F8E58 is a static-analysis candidate used throughout
frame rendering. The live heap scan established the owning renderer object;
its global reference still needs explicit runtime verification before use.

## Native frame entry candidate

Renderer slot 10 / RVA 0x2CEF0 is the large frame-rendering function. It returns
with `ret 0xC`, reads a double from stack argument 1 and a float at stack+0x10,
and writes AL=1 at normal completion. This is a candidate ABI of
`bool __thiscall(renderer, double, float)`, now used by the experimental frame hook, pending game/headset verification.

It has an existing multi-view loop and zeroes the double after the first view.
It also updates wind and other renderer-side managers. Consequently, simply
calling the whole function twice with the original time argument would advance
some presentation state twice. Recover the timing semantics and frame/target
ownership before implementing the pair. The outer simulation must run once.
The current UI/presentation integration is described below. Reflection/shadow
side effects and vehicle/weapon behavior still need observation in gameplay.

## Current native integration (v8)

Camera setter interception keeps native copies from overwriting the selected
world/weapon eye transform while a replay is active. Both view states restore
after each pass. Physical translation is relative to an upright head neutral;
scale/height adjustments and an optional visual grip delta reuse BFVR math.

The native frame calls renderer EndScene (slot 53, RVA 0x06890) and Present
(slot 54, RVA 0x06c20). The D3D9 Present must be suppressed until both eye
readbacks finish; DISCARD invalidates the backbuffer afterward. A final Present
publishes the pair and composites the captured HUD onto the desktop. The
second native render receives zero time delta, matching the native multi-view
loop. Other native render side effects still require observation in gameplay.

## Actual HUD and Flash boundaries

Renderer slot 15 / RVA 0x2ea10 was incorrectly described as interface rendering
in v4. Its queues are RSMT1Person, RSMT1PersonTransparency and their delta
variants. It renders first-person geometry and is no longer the UI hook.

The real HudManager is renderer+0x5cc. Its game-executable vtable is RVA 0x5a6b50;
slot 7 calls exe RVA 0x34e610, observed from native frame RVA 0x2e6d3. Validate
the executable signature/vtable together before installing the draw hook.

Flash Swiff BeginDisplay is renderer RVA 0x117a20, vtable 0x1d7ad8 slot 13,
with nine stack arguments (ret 0x24). EndDisplay is RVA 0x118af0, slot 14. The
hooks preserve all arguments and capture through the final buffered draw.
Nested Flash/HUD boundaries must not detach the outer capture owner. Separate
batches accumulate until the next eye; never clear between HUD and menu draws.

The bot match used 4x MSAA despite earlier profiles having AA disabled. An alpha
capture target must match the current native render target's sample type and
quality. Use a multisampled A8R8G8B8 surface plus a single-sample texture resolve;
GetRenderTargetData and desktop composition happen after the native scene ends.
The GPU tests verify alpha, clean world pixels, native target/depth/viewport
restoration, and successful device Reset for this case.

## Playable-world gate

Renderer global RVA 0x221a58 is PlayerManager. The observed game vtable is exe
RVA 0x5292a0; slot 12 calls exe RVA 0xb7530, bytes 8B 41 6C C3, returning the local
player at manager+0x6c. Validate those relationships and require a local player
before eye replay. Login also has perspective RenderViews (identity at y=100),
so camera validity alone does not mean a playable world exists.

Renderer global 0x221a44 is MainConsole, NOT a menu-state manager. Its slot 41
getter reads +4; do not use it to classify the in-game menu.

## Evidence boundary

The live v7 Verdun diagnostic produced distinct eye buffers, captured bounded
synthetic head movement and remained running through the match transition.
It identified the MSAA UI rejection. v8's matching-target implementation passes
actual D3D9 GPU checks, including the complete session loop with test native
entry points. Its in-game HUD/headset appearance still needs observation.
No controller hardware, native projectile aim, vehicle, or Remaster result has
been established by these diagnostic runs.

## v9 extra Present and menu pointer correction

The user's live v8 match reports successful HUD isolation but repeated neutral
capture log lines. The outer game loop can Present after RenderStereo already
published its pair. Clearing worldSeen at each Present let this second call
publish the whole scene as a menu and flip previousGameplay to false. Use the
validated local-player lifetime (NativeWorldActive) independently of temporary
camera eligibility. The GPU loop now issues that extra Present after all 60
pairs and requires exactly one neutral capture.

MenuPointer uses tracked right aim against the existing 1.6 x 1.5-metre
width/distance quad, publishes the same explicit anchor, draws the beam in both
eye images and the dot into UI, and positions the native OS cursor inside the
foreground game. Since v12, Trigger/A send mouse messages to the game-owned
Flash listener, with entry-held suppression; gameplay input stays in DirectInput.
The user confirmed menu clicks and paused-menu rendering work. The previous
claim that Flash needed DirectInput clicks was incorrect.


## v14 infantry camera comfort

Use the absolute body + local input heading, with the observed horizontal
recoil increment excluded, as a level tracking-space base. Never reuse an
animated or previous VR eye orientation as that base. HMD pitch/roll/yaw remain
physical; source position, movement and stance are native. The same base is
converted into skeleton coordinates for tracked hands, and pinned for both
eye passes. Preserve native bone 0 for firing-reference calculations.

NativeComfort profiles the local soldier heading fields and the exact recoil
getter call at exe RVA 0x18acf9 (return 0x18acfe, target 0x186140). Observe the
float return without modifying it; subtract only its accumulated contribution
from VR heading. The known local actor, parent/attachment checks, rigid matrices
and a bounded camera distance must pass. Unsupported/vehicle views retain the
existing camera. See the v14 handoff for detailed relationships and evidence.

The slow-world-rotation report has not been reproduced in a new headset run.
These changes address animated/tilted camera composition, recoil-driven heading
and false recentering from missing controller samples; automated tests cannot
establish which was the user's perceived cause or confirm its disappearance.

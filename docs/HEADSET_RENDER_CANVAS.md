# Opt-in headset render canvas

This is an unreleased developer experiment. Regular launchers and the installer
retain their accepted source resolution. Compilation does not establish headset
image quality or performance.

## Why source dimensions matter

The existing runtime-sized OpenXR swapchain can receive an upscaled 1600x900
native eye image. Enlarging that destination does not recover scene detail.
The experiment queries the active runtime before the game starts and applies
its recommendation to the actual native render canvas.

Design reference: [Gawkyorange5's BF2-VR resolution handoff](https://github.com/Gawkyorange5/BF2-VR/tree/5614c0b5936705da44fa1e432ad5e26c6ff2bd77/res-fix).
Credit Gawkyorange5 for sharing the startup/window separation approach.
BF2142's implementation uses its own launcher and independently profiled native
window procedure. No BF2 addresses, source snapshots or game assets are bundled.

## Developer launch

Use matching candidate x86 client/launcher and x64 presenter:

```powershell
.\BF2142VRLauncher.exe --game-dir 'D:\Games\BF2142' --windowed --presenter 'D:\Candidate\x64\BFVRPresenter.exe' --headset-resolution
```

`--headset-resolution` is explicit opt-in. The matching presenter queries
recommended and maximum per-eye dimensions using the same OpenXR application
identity as gameplay. A common proportional bound limits either axis to 3072
and the area to 8,388,608 pixels. This may be below the runtime recommendation.
Query failure falls back to the existing 1600x900 source; the query is bounded
to 15 seconds. Restart to query a changed runtime quality preset.

`--render-canvas 2528x2704` provides a bounded manual diagnostic alternative.
Do not combine render-size options. The older `--render-size` behavior is
unchanged. These canvas options are rejected for flat network observers.

## Window, rendering and input

The physical preview fits within a 1600x900 client area and available monitor
space. BF2142 receives the logical render dimensions in non-minimized WM_SIZE
notifications. Client mouse messages map to that logical canvas; wheel and
non-client screen coordinates are untouched. Device creation and reset both
receive explicit backbuffer dimensions, including when native parameters use
zero to request the client size. Existing AA fallback preserves those dimensions.

The new optional producer flag makes the UI display as widescreen content in
the runtime UI texture. Pointer hit testing uses the same content rectangle,
and the custom controls text is authored at that aspect before raster mapping.
Sampling still uses the actual source pixel dimensions. Runtime eye FOV and
poses do not derive from desktop or menu aspect.

In desktop simulation, the OS mouse is the menu pointer. The derived simulated
controller ray must not move the OS cursor again. F9 can still test virtual
clicks, and actual headset controllers retain ray-based menu input.

## Evidence and remaining checks

- Both architectures build; all 75 Win32 CTests pass.
- `BF2142VRNativeRenderCanvasTests --gpu` verifies actual 2528x2704 D3D9
  backbuffer/viewport creation and reset separately from the desktop window.
- `BF2142VRUiCanvasSmoke` verifies a real portrait source presents in the
  intended widescreen rectangle with transparent padding; legacy layout remains.
- An isolated BF2142 desktop run logged matching 2528x2704 backbuffer/capture,
  1600x900 preview and 8x samples. This does not prove both submitted headset eyes.

Before enabling by default or publishing a player build, test a connected headset:

1. Match query recommendation, bounded request, actual source/capture and both
   submitted eye rectangles. Repeat after a materially different runtime preset.
2. Check login, soldier selection, loading Join, deployment, pause and custom
   controls, with desktop mouse and headset laser. Check readable proportions.
3. Check infantry/vehicle views, scope/HUD, minimize/restore and map transitions.
4. Compare clarity and frame times at a fixed scene, including filter settings.
   Higher source resolution adds GPU/readback/upload work and can reduce FPS.
5. Validate disconnected-runtime fallback and the exact staged installer before
   any release. Retain the accepted build as the fallback.

This work does not change simulation timing, IK, pose networking or stock assets.
It does not establish whether residual softness comes from FXAA, bloom, native
postprocessing or headset streaming. Those need separate comparisons.

## Performance rejection and transfer follow-up

The initial larger-canvas desktop playtest was rejected for severe slowness.
Do not promote it based on dimensions or passing unit tests. Opt-in
`BF2142VR_FRAME_PROFILE=1` reports inclusive render-stage times every 60 frames;
it is disabled by default and does not modify engine timing or cap settings.
Initial continuous gameplay samples spent about 3-5 ms in native world draw calls,
22 ms in eye readback, 10 ms in UI readback and 33 ms in desktop publication.
These are CPU wall-clock sections; readback can include waiting for GPU draws.

FramePixels now uses row copies/SSE2 for exact pixel-format conversion. Desktop
composition reuses its buffer and has exact transparent/opaque fast paths.
Tests compare all alpha values, channel conversions, alignment and row tails
against the previous scalar operations. Scope/reflex desktop GPU fixtures pass.
An isolated hardware benchmark at 2528x2704 with 8x MSAA reduced sparse HUD
composition from about 29 to 4.5 ms; single-source readback from 9.5 to 7 ms.
These are individual-operation measurements, not game FPS or headset results.
Synchronous GPU/CPU transport remains a material cost. Gameplay acceptance and
performance validation after this optimization are still pending.

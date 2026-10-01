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


## Comparison with BF1942 / BF2 and GPU-only feasibility (unreleased)

The optimized native-game desktop profile still measured about 26-28 FPS at
2528x2704 / 8x MSAA: roughly 18-19 ms eye readback, 6 ms UI readback and 7-8 ms
desktop publication. Scene differences prevent a controlled FPS comparison
with the earlier rejection. These are CPU wall times, not GPU draw timestamps.

BF1942's D3D8-to-9 translator creates shared D3D9Ex textures and implements
managed-pool compatibility. BF2142 currently uses ordinary D3D9, reads images
into CPU vectors and uploads them through SharedTextureProducer. Ordinary D3D9
rejected the tested shared-render-target allocation on the development GPU;
D3D9Ex BGRA sharing succeeded. Replacing the factory with Ex alone is unsafe
because its managed-pool semantics differ. Do not make that blind substitution.

BF2's reviewed source at 5614c0b5936705da44fa1e432ad5e26c6ff2bd77 still transfers
pixels through CPU memory, but batches eyes and HUD into an atlas, uploads the
mapped data directly, and flushes once. It also has a driver-checked UMA path.
See [DirectAtlasCapture.inl](https://github.com/Gawkyorange5/BF2-VR/blob/5614c0b5936705da44fa1e432ad5e26c6ff2bd77/src/RFX/DirectAtlasCapture.inl)
and [XrSessionClient.cpp](https://github.com/Gawkyorange5/BF2-VR/blob/5614c0b5936705da44fa1e432ad5e26c6ff2bd77/src/RFX/XrSessionClient.cpp).
This code comparison does not establish BF2's achieved FPS at matching settings.

An isolated RX 9070 XT benchmark with three full-sized 8x-MSAA color images and
current-frame pixel checks measured:

| Transfer at 2528x2704 | Milliseconds |
| --- | ---: |
| Ordinary D3D9, separate readbacks plus vector copies | 19.6 |
| Ordinary D3D9, packed atlas readback plus vector copies | 18.4 |
| Ordinary D3D9, lockable atlas plus vector copies | 30.1 |
| D3D9On12 resolve / unwrap / GPU copy / completion wait | 1.6-1.8 |

The GPU case copies to D3D12 shared textures opened by a matching-adapter D3D11
device. Source ownership is returned with a completion fence after the copy;
resources return to COMMON state. A separate untimed D3D11 staging read checks
every pixel of each current frame, including HUD alpha. Managed textures, vertex
buffers and index buffers can be created and locked using ordinary D3D9 semantics.
This is a feasibility result, not a full rendering benchmark or a 144-FPS claim.

Manual developer probes (hidden windows, no game or headset):

```powershell
.\BF2142VRGpuTransferSmoke.exe
.\BF2142VRStereoIntegrationSmoke.exe --on12 --msaa
.\BF2142VRStereoIntegrationSmoke.exe --on12 --desktop --msaa --scope
.\BF2142VRStereoIntegrationSmoke.exe --on12 --desktop --msaa --reflex
```

The --on12 switch selects the system factory only inside this standalone fixture.
Native HUD isolation, distinct stereo eyes, animation time ownership, paused UI,
scope restore and reflex desktop composition pass on that backend. Both builds
and all 75 Win32 CTests pass. No player backend selection or runtime GPU transport
has been implemented in this checkpoint. The accepted launcher and release are
unchanged; do not distribute this as a performance fix.

Remaining integration work must preserve CPU-composited scopes, grenade guides,
menu pointers and custom panels, or move them to equivalent GPU composition.
Producer/consumer ownership must cover actual GPU completion in both directions;
an enqueued copy or Flush alone does not establish safe resource reuse. Exercise
device reset, native shader compatibility, adapter matching, map transitions and
failure fallback before a headset playtest. Keep engine timing and IK unchanged.
144 FPS requires the entire frame, including native rendering and presentation,
to fit within about 6.94 ms; this experiment measures only one part of that work.

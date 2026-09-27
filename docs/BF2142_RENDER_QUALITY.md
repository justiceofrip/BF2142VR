# BF2142 VR render quality

Windowed headset launches use a **1600 x 900 source per eye**. Flat/observer and
simulated launches default to 1280 x 720. The native window, backbuffer and Flash
canvas share one size. The earlier square-source/compact-mirror experiment was
withdrawn because it broke menus; do not restore that path.

## Native geometry antialiasing

The quality hotfix requests real **8x MSAA** when creating/resetting
the VR D3D9 device, with 4x/2x fallback according to color/depth support. The
owned windowed game reported 8x in its video menu but created an unmultisampled
source; the new path records the actual swapchain sample count. It requires the
native discard swapchain and automatic depth allocation, preserves formats and
size, and retries the original parameters on creation/reset failure. Unknown
manual-depth/lockable configurations remain unchanged. Flat observers retain
native settings.

`WorldMSAASamples=8` in `[VR]` selects this default; `4`/`2` reduce cost and `0`
leaves native AA unchanged. Restart the game after changing it. Keep world FXAA
enabled in the presenter; it complements geometry MSAA. Neither path changes
Steam Link compression or magically adds detail to old textures. Transparent
foliage and shader shimmer may still need additional work.

The GPU check draws diagonal geometry with 0/2/4/8 samples and verifies partial
edge coverage survives the same MSAA resolve/readback used for eye transport.
UI capture separately preserves native color/depth/viewport state and alpha.
Actual headset appearance and performance require a headset play session.

## Explicit source size

`--render-size WIDTHxHEIGHT` overrides the launcher source size (640-3072 per
axis). It is not a headset-native resolution setting. OpenXR controls output eye
swapchains and FOV. Raising this size increases native rendering, CPU readback
and transfer cost; keep the verified widescreen menu layout as the baseline.

Texture upscaling remains an optional separate experiment. No generated or
proprietary game textures are bundled with this source or candidate.

## Player settings

Use the updated launcher: its source size overrides the in-game resolution at
startup. Changing the video menu alone does not raise that source. No special
in-game preset is required. SteamVR supersampling can raise the output size,
but cannot recover detail absent from the source. Leave texture quality at your
preferred level; texture enhancement is separate and lowest priority.

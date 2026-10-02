# BF2142 VR render quality

## Test.8 source resolution and presentation

The installed Play VR launcher queries the **active OpenXR runtime's recommended
eye size** before the game starts. This includes that runtime's resolution scale;
it is a recommended render size, not necessarily the physical panel pixel count.
For the 32-bit renderer the source is bounded to 3072 pixels per axis and
8,388,608 pixels per eye, preserving its aspect ratio. If the query fails,
the launcher retains the 1600x900 fallback. Connect the headset before launching
and restart the game after changing runtime resolution.

The desktop preview remains a compact widescreen window. Its input coordinates
and the native menu canvas are mapped separately from the eye source. Changing
the in-game resolution does not replace the launcher's source selection. Native
texture/detail settings still apply; this is not a texture replacement pack.

Ordinary world frames use native D3D9Ex shared GPU textures. Managed-resource
staging preserves native row layout, and reset/COM dispatch corrections preserve
menu fonts and ground textures. CPU fallback uploads reuse staging allocations
instead of accumulating driver upload memory in the 32-bit process.

## Filters and antialiasing

BF2142's added FXAA defaults **off**, following community comparisons that
isolated it as the source of fuzzy text and edges. `bf2142_fxaa_enabled` in the
presenter's `runtime/x64/UserConfig.txt` controls this separately from the older
`fxaa_enabled` used by other BFVR games. Bloom remains independent.

The VR client requests real native **8x MSAA**, with supported 4x/2x fallback.
`WorldMSAASamples=8` under `[VR]` in `BF2142VR.ini` selects that default; 4 or 2
reduces cost, while 0 preserves native AA behavior. Restart after changes.
MSAA does not remove all transparent-foliage shimmer, enhance old textures,
or remove wireless compression artifacts.

## Validation and limits

The owner accepted clear, smooth Quest 3/Steam Link/SteamVR gameplay at a
2064x2208 per-eye source, actual 8x MSAA, and 90 Hz. Repeated unscoped batches
tracked that cadence; heavier scenes dipped below it. This is not a promise
of constant 90 FPS, 120/144 Hz performance, or compatibility with every headset.
Login, readable menus and clean terrain succeeded in the accepted session.

ADS still reads GPU eyes back to the CPU for compositing, and magnified optics
render another native view. This slower path and stray optic HUD graphics are
separate follow-up work. No ADS performance fix is claimed in test.8.

For explicit developer comparisons, `--render-canvas WIDTHxHEIGHT` selects a
bounded source with separate menu/window mapping. `--render-size` alone changes
the older shared window/source size and is not the equivalent of runtime sizing.
Diagnostics remain off by default. The filter comparison helpers under
`scripts/bf2142` can override FXAA/bloom using a temporary configuration.

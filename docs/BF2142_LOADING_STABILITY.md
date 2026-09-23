# BF2142 loading stability investigation

v29 addresses a reproduced legacy shader memory failure, independently of OBS.
The retail shader loader crashes after a failed D3DX effect creation returns no
error buffer; another dump faults inside d3dx9_29 when a high address is mistaken
for a tagged handle. Both were observed before the new OBS OpenVR plugin.

A standalone LAA Win32 fixture reserves the free lower address space and pushes
the old CRT heap above 2 GiB. The original runtime then returns 0x8007000E. The
fix keeps D3DX-owned allocations in reserved low memory while leaving the host
and graphics driver's allocation routines unchanged. All 1,659 locally cached
effects were held concurrently and released, twice, under this pressure. The
original runtime fails at the first effect. An original complex shader also
compiles 20 times successfully under pressure with the fix enabled.

The adapter is limited to the verified runtime layout and original import
pointers. Memory is committed on demand, not all upfront. Failed profile or
reservation checks leave the runtime unchanged. This addresses the reproduced
failure; it does not establish that every possible loading crash is eliminated.
The previous v28 build remains available for rollback.

Microsoft documents the effect runtime's special handling of high addresses
in [D3DXFX](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxfx).
Blindly adding its newer LARGEADDRESSAWARE flag is not an adequate fix for this
legacy runtime and its caller's string/handle conventions. The port preserves
those conventions by keeping the affected allocations below the boundary.

OBS's new direct right-eye capture recorded and remuxed successfully at
1920x1080/60 using AMD H.264 CQP18. The owner's short recording logged 0.3%
rendering lag and 0.3% encoding lag, with audio-buffer warnings around the game
crash. This is evidence that the recording saved, not proof of sustained
recording stability. The game still internally renders at its configured
1280x720; 1080p output does not add source detail.

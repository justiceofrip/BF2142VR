# Third-Party Notices

BFVR includes or is built from the following third-party components:

- **OpenXR Loader 1.1.61**, Khronos Group. The packaged 64-bit loader is used
  to connect BFVR to the active OpenXR runtime. Its license is installed as
  `licenses\OpenXR-Loader-1.1.61.txt`.
- **d3d8to9 1.15.1**, Patrick Mours, with BFVR-specific changes. It translates
  Battlefield 1942's Direct3D 8 calls for BFVR's graphics path. Its license is
  installed as `licenses\d3d8to9-LICENSE.md`.
- **MinHook 1.3.4**, Tsuda Kageyu and contributors. It is compiled into the
  BFVR client for narrowly scoped runtime hooks. Its license is installed as
  `licenses\MinHook-LICENSE.txt`.

- **Opus 1.6.1**, Xiph.Org Foundation and contributors. Statically linked for
  BF2142 proximity voice. See
  `third_party/opus-1.6.1/COPYING`; staged builds include `licenses/Opus-1.6.1.txt`.
  Source: https://downloads.xiph.org/releases/opus/opus-1.6.1.tar.gz
  Archive SHA-256: `6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1`.

The full corresponding third-party source used for BFVR v1.0.0 is retained in
this repository under `third_party`.

- **.NET Native AOT 10.0.12**, Microsoft and contributors. Community helper runtime; MIT and third-party notices are in `licenses/DOTNET-LICENSE.txt` and `licenses/DOTNET-THIRD-PARTY-NOTICES.txt`.

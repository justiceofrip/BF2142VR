# BF2142 VR alpha package readiness

First version: 0.1.0-alpha.1, based on the private v30 runtime. A portable ZIP
candidate now exists; no public upload or GitHub release has been made.

## Completed package work

- Setup selects BF2142.exe and installs under game/BF2142VR, with a separate
  Alpha shortcut. The ordinary working development shortcut is preserved.
- Ship x86 launcher/client, x64 presenter, OpenXR loader and required assets.
- Bundle the Python/Pillow setup utility: recipients need no developer tools
  or runtime downloads. Include all component licenses and upstream credit.
- Rebuild the accepted weapon repairs, body equipment and walker hangar from
  the recipient's own stock installation. No proprietary game archives or
  generated artwork are shipped. Remove local PDB paths from runtime metadata.
- Record original weapon archive and optional LAA executable-header changes.
  Restore only expected versions; later mod changes and corrupt backups stop
  uninstall before it restores any file. Backups/settings are retained.
- Include START HERE, controls, troubleshooting, release notes, checksum and
  a validation report. The source tools are included in the player payload.

## Checks completed on the staged candidate

The underlying v30 Win32/x64 builds, 60 CTest and 13 GPU fixtures passed.
The candidate adds 15 mesh-policy tests and actual standalone asset generation:
the repaired archive matches the accepted v25 SHA-256. Its regenerated lobby
passes the GPU fixture and was visually inspected. Actual fresh installation
into a separate game path with spaces passed, including enabling LAA from an
unflagged test EXE. Native launcher inspection and repeat setup passed.
Uninstall refused a later mod change and a corrupt backup without partially
restoring other files. Normal and repeated uninstall passed; the recorded game
files matched their initial hashes exactly. A second install from the actual
ZIP extraction also passed with the previously repaired weapon archive.

## Still required before public release

- Run the exact installed package through a real match/load/respawn and headset
  acceptance session. Source/GPU tests do not establish this packaged run.
- Keep multiplayer, remote IK, Remaster and other runtimes explicitly unverified
  unless their separate acceptance work is completed.
- Publish only the original staged ZIP/checksum, never the installed BF2142VR
  folder: installation creates private game assets and backups in that folder.

The candidate is saved for this final playtest. Await owner readiness for the
packaged headset run; do not terminate their active game or infer acceptance
from elapsed time. Do not hold the first alpha for manual reloads or remote IK.

**BF2142 VR roadmap**

Public baseline: 0.1.0-alpha.1 (v30), available as an early playtest. Additional
multiplayer work lives on the development branch and is not in the public ZIP.
The one-PC dedicated lab has exercised tracked firing and visible remote arm
movement through an explicit bot mirror. Two actual VR clients and external
networking still need their own acceptance.

**First public alpha**

- Produce a portable launcher/installer with game-folder selection, complete
  runtime dependencies, a setup guide, controls, known issues and uninstall.
- Generate weapon repairs, holster equipment and walker-lobby assets from each
  user's own installed game. Keep personal paths, accounts, logs and extracted
  game files out of the download and source repository.
- Preserve upstream MIT attribution and dependency notices.
- Verify the exact staged payload on a fresh installation path and confirm
  uninstall/rollback. Existing source/GPU tests do not establish package setup.
- Keep the initial supported scope to the tested singleplayer/bot setup unless
  multiplayer gets its own successful test before publication.

**Online play and community matches**

- Run a private server with two separate clients. Check controller-directed
  server-registered shots, damage, deaths/respawns and Titan boarding/objectives.
- Investigate a community Titan server and scheduled play sessions. Hosting
  alone does not add VR aim synchronization or replicated arm poses.
- Add a bounded, player-identified head/hand pose channel and third-person IK
  for viewing clients. Keep visual pose replication separate from authoritative
  gameplay. Define behavior for ordinary clients and missing/stale VR poses.
- The public alpha shows stock remote animations. Experimental remote arm IK
  exists in the private lab, with desktop bot-mirror verification. Confirm it
  between two real VR clients before advertising multiplayer waving.

**Voice chat - deferred**

- Start with BF2142's native squad voice and add a VR push-to-talk binding.
- Verify Quest/Steam Link microphone routing, receive audio and two-client
  squad communication on the dedicated server. Native server voice is present
  but disabled in the current private lab; no voice support is claimed tested.
- Consider proximity/spatial voice separately after squad radio works.
- Keep this out of the ADS/holster regression build at the owner's request.

**Physical vehicle controls - requested**

- First target: buggy steering wheels. Grab the wheel with either hand and
  rotate it physically to steer; investigate two-hand grip and hand transfer.
- Map wheel movement to native vehicle steering with a calibrated center and
  range. Keep steering stable when a hand releases or tracking disappears.
- Retain a conventional input option. Consider physical levers/controls for
  other vehicles after a buggy implementation is usable.
- No physical wheel interaction or vehicle cockpit reconstruction has been
  implemented yet. Do not advertise a complete VR vehicle conversion.

**Interaction and compatibility improvements**

- Refine controller-touch finger curl, free/support hands and weapon-specific
  grips without replacing accepted knife and firearm attachments.
- Improve the missing grenade trajectory guide and supported optic alignment.
- Refine grip-and-pull traversal and mounted camera transitions with feedback.
- Check Project Remaster/HD assets and additional OpenXR/headset combinations.

**Optional manual reloading - requested**

- Investigate magazine removal/insertion and charging/bolt interactions per
  weapon, while retaining button reload as an option.
- Preserve the native ammo/inventory rules and verify the multiplayer behavior.
  Scope depends on usable model parts and reliable native reload hooks; this
  is a future feature, not present in the current build.

**Longer-term experiments**

- Hand-to-hand knife toss/catch or transfer.
- Rung-specific tracked climbing hands instead of native climbing animation.
- Better cockpit geometry and interiors for VR viewpoints.

No dates are promised for the roadmap. Bare-hand optical finger tracking is
not a current objective; the requested hand expressiveness uses controller
capacitive touch and analog inputs.

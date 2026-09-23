# Contributing

Build both architectures and run the complete Win32 CTest suite with Build-BF2142.ps1. For mesh changes, also run the Python tests in docs/BUILD_BF2142.md. Preserve supported game profiles and fail closed when native signatures do not match. Keep experimental game profiles separate from accepted ones.

Read AGENTS.md and docs/AI_DEVELOPER_HANDOFF.md before changes. Historical BF1942 addresses, BF2142 offsets and skeleton assumptions are not interchangeable. BF2 work should introduce an explicit game adapter and retain the working BF2142 path; see docs/BF2_PORTING.md.

Describe the changed behavior, relevant tests and what was actually tried in a headset. Automated tests establish math and state handling, not headset comfort or multiplayer compatibility. Submit focused changes with reproducible failure details. Never commit private game files, generated artwork, account data, logs or build output.

Bug reports should include build/version, headset/runtime, GPU, game version and mod, map, steps and a short clip if useful. Review logs for personal information before attaching them. For multiplayer, distinguish a successful connection from verified hit registration and remote motion replication.

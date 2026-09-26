# Multiplayer voice — beta

The beta includes native loopback voice and an authenticated internet bridge in `src/community`. Native adapters remain loopback-only; expose only the encrypted bridge. The owner confirmed proximity voice worked with the second vrtester client. Wider microphone routing/quality on separate PCs needs testing; an earlier outside-player report had no audible voice. See [hosting](BF2142_COMMUNITY_HOSTING.md).

## Channels

- Proximity: voice activation, distance falloff and stereo direction. Alive,
  focused VR players with fresh tracking transmit without holding a button.
  Menus, focus/tracking loss, death, proximity mute or radio PTT stop capture.
- Squad radio: grip the upper-left shoulder radio with the left hand; its thumb
  switch emits BF2142's native squad PTT (`V`). Release to stop transmitting.
  Native squad-leader/commander (`B`) behavior is retained. Both native PTT keys
  pause proximity transmission to avoid sending the same speech on two channels.
- Ordinary unmodified flat clients retain native squad/commander voice. They
  need the compatible addon for this custom proximity channel and VR poses.

## Player controls

Open **VR Controls -> Voice** (Insert also opens VR Controls): proximity on/off,
proximity microphone mute, activation threshold, proximity volume, microphone
and output device. Click device rows to cycle; Default uses Windows' default
communications endpoint. Set that endpoint to Steam Streaming Microphone / your
headset output before opening the game for native BF2142 voice as well.

These controls affect proximity. Native radio retains the game's own settings.
Device numbers are local to this Windows installation; if devices change, select
Default or reselect the named microphone/output. Proximity has no echo
cancellation in this prototype; use a headset for normal play. The one-PC
observer is explicitly receive-only so it never opens another microphone.

Settings persist under `[VR]` in the selected `BF2142VR.ini`:

```ini
ProximityVoice=1
ProximityMicMuted=0
VoiceThresholdDb=-40
VoiceVolume=1.0
VoiceInputDevice=4294967295
VoiceOutputDevice=4294967295
```

More negative thresholds make voice activation more sensitive. Range is -65 to
-15 dB; volume is 0 to 2. `4294967295` selects the communications default.

## Host policy

The private `BF2142VR_NETWORK` file also contains the existing pose-channel
configuration. Add this section to enable proximity, then restart the server:

```ini
[Voice]
Enabled=1
Port=17569
RangeMetres=20
EnemyProximity=1
```

`EnemyProximity=1` allows any nearby player, including enemies. Set `0` for
teammates only. Range accepts 2-100 metres. Team-only filtering fails closed
when the native team profile cannot be verified. Disabled or malformed voice
configuration leaves the ordinary game/pose path available.

Positions and teams come from the server's native roster. Voice endpoints must
match a session already admitted through the private pose/subscription channel;
leases, sequence checks, bounded buffers/work and rate limits discard stale,
replayed, mismatched-owner or excessive packets. These loopback protections are
not an Internet authentication/encryption design.

## Implementation and verification

`src/bf2142/voice` uses pinned Opus 1.6.1, mono 16 kHz, 20 ms frames, 24 kbit/s,
bounded receive queues, short loss concealment and WinMM communication devices.
Audio runs on its own worker, not the game's render thread. Raw microphone audio
is not saved; diagnostics expose counts and levels only.

Automated tests exercise real UDP relay fan-out, Opus encode/decode, voice
activation, spatial gains, enemy/team and distance rules, rate limits,
owner/session/replay/lease rejection, guarded native team reads and UI settings.
The private candidate also passed real Windows playback and the D3D9 stereo
fixture. Two real game clients on Suez co-op then passed a synthetic-tone check: the
primary captured through VB Cable, sent 62 Opus frames, and the observer received
all 62 and submitted 63 decoded/concealed frames for playback, with no device
errors. Quest microphone routing, native squad PTT and headset sound/latency
still require acceptance; test-tone delivery is not that acceptance.

No acoustic wall occlusion, automatic radio-device switching, network finger
retargeting or physical commander-radio gesture is implemented in this version.

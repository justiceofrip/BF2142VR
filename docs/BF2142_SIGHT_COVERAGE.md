# BF2142 sight coverage — private candidate

Full stock v1.51 audit, 2026-10-02. This is implemented/tested coverage, not a claim
every new calibration has been accepted in a headset. The accepted scope-HUD build
remains saved separately. Custom weapon packs require their own profiles.

The installed-game audit checks 86 GenericFireArm templates: **24 optical zoom
profiles, two iron-sight pistols, and 60 non-optical weapons/attachments/equipment**.
Both the original owned install and the repaired private VR install pass.

## Optical profiles

All rows below have an exact stock native-factor match, a routed native zoom HUD,
valid eye-relief/projection checks and CPU/GPU rendering comparison coverage.
Existing EU/PAC assault/sniper, unlock sniper and EU LMG profiles keep their
accepted calibration. All newly added placements still need visual acceptance.

| Weapon | Template | VR presentation |
| --- | --- | --- |
| SAAW 86 Anti-Air | `as_aa` | 2x optic with native zoom HUD/fallback reticle |
| Krylov FA-37 AR | `as_ar_rifle` | 2x optic with native zoom HUD/fallback reticle |
| PK-74 AR-Rocket | `as_ar_rocket` | 2x optic with native zoom HUD/fallback reticle |
| Sudnik VP | `as_av` | 1.5x optic with native zoom HUD/fallback reticle |
| Shuko LMG | `as_mg` | 1x optic with native zoom HUD/fallback reticle |
| Malkov RK-11 SMG | `as_smg` | 1x red dot with clear lens |
| Park 52 Sniper Rifle | `as_sni` | 4x optic with native zoom HUD/fallback reticle |
| SAAW 86 Anti-Air | `eu_aa` | 2x optic with native zoom HUD/fallback reticle |
| SCAR 11 AR | `eu_ar_rifle` | 2x optic with native zoom HUD/fallback reticle |
| PK-74 AR-Rocket | `eu_ar_rocket` | 2x optic with native zoom HUD/fallback reticle |
| Mitchell AV-18 AV | `eu_av` | 1.5x optic with native zoom HUD/fallback reticle |
| Bianchi LMG | `eu_mg` | 1x optic with native zoom HUD/fallback reticle |
| Turcotte Rapid SMG | `eu_smg` | 1x red dot with clear lens |
| Morretti SR4 Sniper Rifle | `eu_sni` | 4x optic with native zoom HUD/fallback reticle |
| Zeller-H Advanced Sniper Rifle | `unl_adv_sni` | 4x optic with native zoom HUD/fallback reticle |
| Pilum H-AVR | `unl_av_rifle` | 2x optic with native zoom HUD/fallback reticle |
| Promotional rifle | `unl_best_buy_rifle` | 2x optic with native zoom HUD/fallback reticle |
| Promotional rifle rocket | `unl_best_buy_rocket` | 2x optic with native zoom HUD/fallback reticle |
| Lambert Carbine | `unl_carbine` | 1.5x optic with native zoom HUD/fallback reticle |
| Baur H-AR | `unl_har_rifle` | 2.5x optic with native zoom HUD/fallback reticle |
| Baur rifle-mounted rocket | `unl_har_rocket` | 2.5x optic with native zoom HUD/fallback reticle |
| Ganz HMG | `unl_hmg` | 2x optic with native zoom HUD/fallback reticle |
| Voss L-AR | `unl_lar_rifle` | 1.5x optic with native zoom HUD/fallback reticle |
| Voss rifle-mounted rocket | `unl_lar_rocket` | 1.5x optic with native zoom HUD/fallback reticle |

## Native sights retained

`eu_handgun` and `as_handgun` retain their physical iron sights; they have no
separate stock optical zoom geometry. They do not get a floating scope overlay.

The non-zoom shotgun variants remain native: `unl_shotgun`, `bp1_expl_shotgun`,
`eu_ar_shotgun`, `as_ar_shotgun`, `unl_har_shotgun`, `unl_lar_shotgun`, and
`unl_best_buy_shotgun`. Knives, explosives, support equipment and passive
upgrades likewise do not acquire an invented optical scope.

## Rendering and gameplay boundaries

- SMG and PAC LMG lenses are opaque mesh surfaces. Their 1x sight needs one
  extra world view on the GPU to show through the lens. Transparent EU LMG
  reflex glass remains dot/HUD-only without another world view.
- The AA zoom root, including native reticle/lock information, replays inside
  the optic. Native lock acquisition and firing logic are unchanged.
- Rifle-mounted rockets are separate native weapon objects and have explicit
  entries. Their rangefinder and stock rocket artwork use the same lens as
  the parent rifle; no new ballistic compensation or targeting authority is added.
- No server asset change, weapon unlock grant, IK edit or SteamVR setting change.

## Repeat the read-only inventory check

From the repository root, using Python 3.9 or newer:

```powershell
python scripts/bf2142/AuditWeaponOptics.py --game-dir "D:\Games\Battlefield 2142" --report sight-audit.json
python scripts/bf2142/TestAuditWeaponOptics.py
```

The audit reads owned `Weapons_server.zip` and `Menu_server.zip`, matches source
profiles to stock zoom factors and exact HUD roots, and exits nonzero for missing
optics, mismatches, missing routing or stale definitions. It does not modify or
copy game content. Do not commit installed-game archives or generated meshes.

## Automated validation

- Both architecture builds and all 76 Win32 CTests pass.
- Six audit fixtures check omissions, wrong factors, stale definitions, HUD
  routing, explicit iron-sight exceptions and archive/comment parsing.
- Native optics tests enumerate every profile, including heap-backed long names.
- GPU/CPU image comparisons enumerate every profile: 96 cases at 512x512 and
  another 96 at 2064x2208, covering both eyes and native/fallback HUD artwork.
- Scope/menu integration checks cover magnified, transparent-reflex and opaque
  1x paths, with exactly one native animation-time advance per gameplay frame.

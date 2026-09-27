# Player feedback and current work

Reported 2026-09-27, primarily from Alpha.1 with Beta.1 setup attempts.

| Report | Status / next action |
| --- | --- |
| Setup fails inconsistently, sometimes only “Setup did not finish”, sometimes “function object is not iterable”, sometimes during weapon repair | Open. Preserve the exact error; add persistent worker logs/tracebacks and investigate the failing stage. Origin/EA App + Reclamation reported. Do not call repeated retries a fix. |
| Low-resolution, blurry VR; in-game resolution/AA settings unclear | Quality hotfix: 1600x900 headset source, explicit render-size override, actual native MSAA with capability fallback. See render-quality guide. 8x source and readable menus checked on desktop; headset performance still needs feedback. |
| Enabled crosshair does not follow tracked gun | Confirmed design gap: existing toggle controls the flat HUD crosshair, not a gun-directed world reticle. Default stays hidden. Track a proper controller/bore-directed alternative separately. |
| VR CONTROLS overlaps native Exit X | Fixed in quality hotfix: move button and hit region to top center; right-hand exit area no longer intercepted. |
| Severe geometry jaggies despite 8x in video menu | Native MSAA fix included; GPU resolve/readback checked at 0/2/4/8x. Transparency shimmer and old textures are separate issues. |
| Native ADS reticle below weapon, missing EU SMG/launcher optics, sniper alignment, launcher grip | Private gameplay candidate; keep separate from urgent quality release until native checks finish. |
| Cannot attach to / climb ladders | Private candidate fixes stick input suppressed by grip gestures; attachment still requires reproduction. |
| Muddy textures / upscale comparison | Lowest priority; optional experiment only, no texture pack installed or distributed. |

These reports do not establish that every EA/Reclamation installation fails.
An installer log from a failing machine is needed to attribute the intermittent
failure; synthetic checks alone cannot prove it solved.

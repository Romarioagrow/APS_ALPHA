# Dev Surface APS-74 canonical parity baseline

Date: 2026-08-14

Result: **PASS for baseline identity/evidence capture only**. No new orbit-to-ground visual comparison is claimed.

## Canonical primary truth

- Body key: `SYS0/S0/P0` (`MakePlanetKey(0, 0)`, `StartPlanetIndex=1`).
- Generated/model surface seed: `424242`.
- Type/family: `EPlanetType::Water` / `Oceanic`; ocean enabled.
- Radius: 6371 km.
- Feature, Relief, Land Coverage, Mountain, Crater and Roughness controls: all `1.0`.
- Preview profile signature: `4272571861`; noise scale/intensity `190 / 870`; terrain `MI_APS_WS_Oceanic`; water `MI_APS_WS_Water`.
- Full-scale gameplay: `planetScale=6.371e8 cm`, `noiseIntensity=461872`, terrain `10 x 96 @ 120 cm`, ocean `9 x 64 @ 200 cm`, runtime collision `64 x 64 @ 120 cm`.
- This Water/Oceanic seed is the parity truth for future same-camera orbit-to-surface comparison. It must not be replaced by a gallery fixture.

Evidence: `Saved/Logs/CodexWetOceanHandoff_CleanLivingWater_20260814.log`, especially lines 1035, 1075-1077, 1135-1145 and 1193.

## Cross-family comparison only

Fixture contract: body `SYS0/S0/P0`, seed `73991`, radius 6750 km; the same six controls, including Land Coverage, are all `1.0` in `APSPlanetSurfaceGallerySmokeTests.cpp` lines 329-342.

| Case | Canonical mapping | Ocean | Profile signature | Noise scale / intensity | Runtime terrain identity | Accepted preview perf |
| --- | --- | ---: | ---: | ---: | --- | --- |
| Frozen | `Ice -> Cryogenic` | no | `1536386311` | `510 / 665` | `MI_APS_WS_Cryogenic` | 108.1 FPS, 14.29 ms max sampled frame |
| Desert | `Desert -> Desert` | no | `18392993` | `514 / 1300` | `MI_APS_WS_Desert` | 111.7 FPS, 13.31 ms max sampled frame |
| Water | `Water -> Oceanic` | yes | `830115675` | `251 / 839` | `MI_APS_WS_Oceanic` + `MI_APS_WS_Water` | 112.4 FPS, 13.08 ms max sampled frame |
| Living | `Forest -> Biosphere` | yes | `396759706` | `585 / 1430` | `MI_APS_WS_Biosphere` + `MI_APS_WS_Water` | 114.2 FPS, 12.80 ms max sampled frame |

Gallery evidence: `Saved/Logs/CodexPlanetGallery_ReferenceLivingWater_20260814.log` and:

- `Saved/Screenshots/Windows/APS_PlanetGallery_Ice.png`
- `Saved/Screenshots/Windows/APS_PlanetGallery_Desert.png`
- `Saved/Screenshots/Windows/APS_PlanetGallery_Water.png`
- `Saved/Screenshots/Windows/APS_PlanetGallery_Forest.png`

## Non-regression gates

- Preserve Ocean/Living allocation and Land Coverage `1.0` while diagnosing material crawl, relief readability or biome diversity.
- A material/LOD fix must not change the primary `424242` body key, type/family, ocean flag, profile signature inputs or resolved terrain/water identity.
- Frozen, Desert and Living must remain distinguishable by canonical family/profile, not by fallback palette alone.
- Compare future performance at the same viewport/camera/settings. Gallery switch spikes are recorded separately and are not steady-state frame-time gates.
- The PNG viewer was blocked by sandbox ACL in this thread, so existing pixel-test PASS is preserved as inherited evidence; no new manual visual PASS is claimed.

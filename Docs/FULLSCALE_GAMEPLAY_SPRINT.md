# APS full-scale gameplay sprint

Branch: `codex/fullscale-gameplay-sprint`

This sprint connects the existing gameplay systems without replacing the project's full-scale architecture. The current result is a functional vertical slice and a stable C++ integration base for the next UX/content iterations.

## 1. World generation is gameplay

The existing `UAstroGenerationMenu` remains the default UMG presentation. Its parameter changes now go through `UWorldGenerationViewModel`, which owns validation, a short preview debounce, preview state, and the final world commit. `SWorldGenerationPanel` is an optional minimal Slate presentation over the same view model.

The view model calls the real `AAstroGenerator`. Preview generation creates the actual scene actors and meshes, focuses the generator's scene camera on their bounds, and suppresses gameplay-only actors. The final Create World action stores the same `UGeneratedWorld` model in `UMainGameplayInstance` before loading `L_WorldGeneration`.

Preview instance counts are capped at 10,000 for responsiveness. This cap does not change the committed world parameters. Large World Coordinates and the existing full-scale representation remain the source of truth.

## 2. Character, gravity, and zero-G

`ACustomGravityCharacter` is the main UE5/Manny-compatible character path. It uses UE 5.4 `UCharacterMovementComponent::SetGravityDirection` and `UGravityDetectorComponent`.

Gravity source resolution order:

1. overlapping ship;
2. overlapping station/headquarters;
3. overlapping orbital body;
4. nearest full-scale world actor whose surface distance is inside `AffectionRadiusKM`;
5. true zero-G when no source applies.

Planets use radial gravity. Stations and ships use their local negative Up axis as planar gravity. Zero-G uses `MOVE_Flying`, camera-relative three-dimensional input, zero gravity scale, and restores normal falling movement when a gravity source appears.

Character controls used by this integration:

- `F`: interact / take ship control;
- `Space`: move up in zero-G (and normal jump under gravity);
- `Left Alt`: move down in zero-G.

## 3. Unified ship interaction and flight scales

All `APilotingVehicle` descendants inherit one native `IVehicleControlling` contract. Entering a ship preserves the pilot controller, collision, tick, physics, and movement state; attaches the pawn to the ship seat; and transfers possession. Releasing returns possession at the ship exit point and inherits the ship velocity.

Existing ship Blueprints can customize presentation through `OnPilotControlStarted` and `OnPilotControlEnded` without reimplementing the control transaction.

Flight mapping:

| Flight scale | Movement mode | Purpose |
| --- | --- | --- |
| Station / Planetary / Basic | Impulse | local physical flight |
| Interplanetary / Stellar | SpaceWrap | move the current star-system frame |
| Interstellar / Intergalaxy | Offset | non-physical large-distance ship offset |

The current controls retain the project's input mappings:

- `F`: leave the ship;
- `G`: switch engines;
- `Right Shift` / `Right Ctrl`: increase / decrease flight scale;
- `Left Shift` / `Left Ctrl`: acceleration / deceleration boost.

Thrust, rotation, braking, damping, and boost interpolation are frame-rate independent. Physical velocity is preserved across physical/non-physical engine transitions.

## Verification

- UE 5.4 `APS_ALPHAEditor Win64 Development`: succeeds.
- `APS.Gameplay`: 4/4 automation tests pass.
  - generation model constraints;
  - character gravity/zero-G transitions;
  - character-to-ship control round trip;
  - flight-scale-to-engine-mode mapping.
- Main menu NullRHI smoke: clean startup/shutdown, correct main-menu GameMode, no non-focusable-widget input error.
- Live preview smoke: real `BP_AstroGenerator` preview created, 10,000 star instances.
- Single Play NullRHI smoke: `BP_GameModeCustomGravity`, generated five-planet home system, start planet integrated, three current ships acquired the generated `OffsetSystem`, clean shutdown.
- Targeted Blueprint validation: 13 gameplay/menu assets compile without errors or warnings.

## Known legacy debt

- The old `BP_SHIP_M-CLASS_CONTROL-FlIGHTS` level actor still reads element zero from an empty `GetAllActorsOfClass` result during Single Play. It does not block startup or the current C++ ship path, but its Blueprint graph should be migrated or removed in a later content pass.
- The project-local AtmoScape source had a malformed atmosphere material reference and is repaired in this checkout. `/Plugins/` is ignored by the repository, so that vendor/dependency repair is intentionally not included in the branch commit and must be preserved in the plugin distribution.
- Existing missing-material and XR-plugin warnings outside the selected gameplay assets are pre-existing content/configuration debt.

## Engine reference

- Epic custom-gravity tutorial: <https://dev.epicgames.com/community/learning/tutorials/w6l7/unreal-engine-custom-gravity-in-ue-5-4>
- Epic `SetGravityDirection` API: <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/SetGravityDirection>

# APOSFERA audio

## Footstep revision - 2026-10-04

Metal now uses five processed LowMetal recordings instead of the loud MetalSteps
recordings. Peaks are capped at -18 dBFS before the existing playback gain;
75 Hz high-pass, reduced ringing mids/highs, and short edge fades soften the hits.
Plastic/linoleum, stone/paving, wood and snow each have five separate recordings.
Grass/dirt keeps its existing recordings. Immediate repetition remains suppressed;
pitch variation is narrowed to 0.98-1.02 after the next native build.

Footstep assets: Audio/Footsteps. Sources, hashes and processing recipe:
SourceAudio/APS/Footsteps and Tools/Audio/Render-Footsteps.py. All recordings come
from the already-owned Footsteps Mini Pack; no new download or license is needed.

### Animation contact timing (source prepared for Rider)

The audio subsystem binds to the controlled skeletal mesh's finalized-bone event,
after blending and IK. It observes the lower heel/toe relative to the animated
root, arms each foot after a 5 cm lift, and emits once on descent below 2 cm.
The 8 cm ankle-to-sole offset is for the current Quinn skeleton. It changes no
animation, mesh, movement, gravity or save data. Grounded/moving checks, a 100 ms
contact guard, landing suppression, and frame deduplication prevent idle chatter,
airborne steps and double hits. Long hitches discard contacts rather than catching up.
Distance cadence is retained only for pawns lacking the required foot/toe bones.

Surface selection uses a short trace under the contacting foot along local gravity.
Priority: authored physical surface, explicit component/actor tag, contacted
material name, known metal structure, default terrain. Only the contacted material
slot is inspected; ambiguous meshes retain their safe fallback.

| Physics surface | Tag | Recording |
| --- | --- | --- |
| SurfaceType1 / APS_Grass | APS.Surface.Grass | Existing dirt/grass |
| SurfaceType2 / APS_Metal | APS.Surface.Metal | Soft low metal |
| SurfaceType3 / APS_Plastic | APS.Surface.Plastic | Linoleum/polymer |
| SurfaceType4 / APS_Stone | APS.Surface.Stone | Paving stone |
| SurfaceType5 / APS_Wood | APS.Surface.Wood | Parquet |
| SurfaceType6 / APS_Snow | APS.Surface.Snow | Snow |

Common material names (plastic/polymer/linoleum/rubber, stone/rock/concrete/pavement,
wood/parquet, snow/ice, metal/steel, grass/dirt/soil) select these sets automatically.
HQ Floor01_Soft selects polymer. Unnamed or ambiguous surfaces can use the explicit
tag or a physical material; this change does not assign materials across the world.

### Verification and next build

Import completed in the existing editor without stopping PIE: 25 SoundWaves,
five updated metal cues and the existing bank were saved. Report/backup:
Saved/AudioIntegrationBackups/footsteps-v2-20261004-021706/footsteps-change.json.
The running editor predates the new physical surface enum names. The native bank
loader now loads the twenty additional waves asynchronously and fills only missing
surface sets; no second import is needed after the Rider build. A fresh editor
can also serialize these mappings directly using Import-Footsteps.py.

The standalone C++ contact detector passed tests on exported MF_Walk_Fwd and
MF_Run_Fwd poses at 30/60/120 FPS and playback rates 1/1.4/1.85, plus idle,
landing, airborne, stopping, hitches and invalid-input guards. This exercises
the actual detector header; it is not a full Unreal build or gameplay acceptance.
Unreal regression tests: APS.Audio.PoseContacts and APS.Audio.FootstepSurfaces.
After the user's Rider build and editor restart, check slow walk/run/start/stop,
deck-to-terrain edges, plastic/stone/wood/snow surfaces, landing and vehicle exit.
Subjective sound quality and the timing of the final blended pose need in-game review.

## Current state — 2026-10-03

### Gameplay - eight electronic space tracks

SC_Music_Exploration now plays SW_Gameplay_CosmicJourney_v1, an independent
gameplay sequence from Stellardrone's Light Years (2013). Eternity (spoken word)
and its reprise are excluded. The cycle contains eight distinct compositions:

| Start | Composition |
| --- | --- |
| 00:00 | Airglow |
| 04:23 | Comet Halley |
| 07:26 | Ultra Deep Field |
| 12:17 | Light Years |
| 16:35 | In Time |
| 19:41 | Cepheid |
| 23:31 | Red Giant |
| 25:52 | Messier 45 |

Cycle length: 27:39. Order is fixed; all eight play before
repeating. Transitions overlap for six seconds using complementary linear fades.
The repeating boundary ends and starts at zero. Opening gameplay starts at Airglow.

Source tracks are gain-matched toward -24 LUFS while respecting a -8 dBFS
pre-cue true-peak ceiling. Higher-crest tracks stay quieter to preserve dynamics.
The resulting measured sample peak is -8.05 dBFS before the cue gain
of 0.65 (about -3.74 dB). Gentle 28 Hz high-pass and -2 dB high shelf at 3.5 kHz
reduce subsonic energy and bright synth edges. No pitch changes, limiting or
dynamic compression. Exact trims, gains and original hashes are in CosmicJourney.json.
These measurements do not establish subjective listening approval.

The music keeps the existing Music sound class and Music/Master preferences.
SpaceAmbience and InteriorAmbience remain unassigned. Vehicles, steps and UI are
unchanged. The gameplay cue and its new wave have been saved in Unreal.

Sources and license verification: SourceAudio/APS/CosmicGameplay. The author links
the official free archive download and licenses the album under CC BY 4.0; the
original CC BY 3.0 notice is also retained. Credits ship in AudioCredits/GameplayMusic.txt
through the existing NonUFS packaging directory. No paid purchase or new plugin.

Reproduce with Python/numpy and the local FFmpeg executable:

    python Tools/Audio/Render-CosmicGameplay.py SourceAudio/APS/CosmicGameplay/Originals output path/to/ffmpeg.exe

After placing the rendered WAV/JSON in SourceAudio/APS/CosmicGameplay, run
Tools/Audio/Import-CosmicGameplay.py in the editor or a Python commandlet. It checks source hashes,
refuses dirty target assets, backs up SC_Music_Exploration, imports only the new wave,
and verifies menu assets and the bank were preserved. Re-running an already applied
import validates the installed duration/gain and makes no changes.

### Menu music retained

SC_Music_Menu still uses SW_MelodicMusic_v2 (A Kind Of Hope, Tears in Rain, Celestial
by Scott Buckley), gain 0.67. Its 16:48 sequence, transitions and retained first
streaming chunk are unchanged. The rejected NewAge/CalmSuite recording stays inactive.
Menu source/credits remain in SourceAudio/APS/MelodicMusic and AudioCredits/MenuMusic.txt.

### Verification for this revision

The user-authorized headless Python commandlet completed with exit code 0,
zero errors and six unrelated project/plugin warnings. It saved the new wave
and updated SC_Music_Exploration. Duration is 1659.0001 seconds, cue gain is 0.65,
and the bank routes this cue through the existing Music class. File hashes confirm
that the menu cue, menu wave, music class and audio bank were preserved.

Import report and cue backup:
Saved/AudioIntegrationBackups/cosmic-20261003-025755/cosmic-change.json.
Commandlet log: Saved/Logs/APS-CosmicMusic-Import.log.

The sources were verified against publisher checksums; the render checks finite
samples, peak headroom, eight unique tracks, and zero-valued loop endpoints.
All eight track entries exceed -46 dBFS RMS over their first eight seconds,
including the cue gain. The commandlet used NullRHI and nosound: an in-game
listening check remains necessary, especially under ship/vehicle effects.
The source bank recipe and new in-menu Stellardrone credit require the next Rider
build; the imported music itself works with the existing compiled audio subsystem.
No native build or interactive editor launch was performed by the audio task.

### Existing Audio settings and pause menu - source prepared for Rider

The existing WBP_SettingsPanel sliders controlled MBLS SC_* classes, whereas
our recordings use APS SCL_* classes. An extra native group above the old panel
therefore left the familiar Audio tab disconnected from game sounds.

APSAudioSettingsBridge now connects the five existing Blueprint values to the
APSAudioSubsystem mix. The main menu attaches immediately after Blueprint
Construct; the world subsystem discovers the MBLS panel used by WBP_PauseMenu
every 0.25 seconds, including while the world is paused. Only these two authored
panel classes are considered. No Blueprint assets or graphics controls change.

The bridge observes the panel's values each UI frame, including changes made by
mouse, keyboard/controller and the existing Reset action. Preferences populate
the original sliders and percentage labels on opening. Changes save after
0.35 seconds without input and flush when the panel closes. APSAudioUser.ini
remains authoritative; the existing Save_AudioSettings function also keeps the
template's audio save synchronized for its Reload/Restore actions.

| Existing slider | Active game channel |
| --- | --- |
| Master | All APS sound classes |
| Music | Both menu and gameplay music |
| Ambient | Environment channel; currently intentionally silent |
| SFX | Footsteps, landing, ship, rover, hover and drone |
| UI | Interface clicks, hover and confirmations |

The duplicate native sliders are removed when the authored panel is connected;
credits remain below it. A schema mismatch logs a warning and leaves the native
fallback controls available in the main menu. The integration validates field
types, row widgets, slider ranges and the Blueprint setter/save signatures.

Not built or run: Rio owns the next Rider build. After restarting, run
APS.Audio.SettingsWidgetContract (both real Blueprint widget trees and their
normalized slider setters, without touching saved preferences). Then check:

1. In main-menu Audio, Music 0 silences music; returning it restores playback.
2. Master 0 silences music and UI. UI 0 leaves music playing but silences clicks.
3. In gameplay pause settings, SFX 0 suppresses steps and each vehicle engine
   after resuming; Music remains independent. Repeat mute/unmute there.
4. Close/reopen settings, travel between menu/gameplay, and restart the game:
   slider percentages and audible levels must retain the selected values.
5. Test the existing Restore defaults action and keyboard/controller changes.

### Footstep startup correction — source prepared for Rider

The old cadence waited a full stride before the first sound: 85.4 cm at the slow
walk speed of 170 cm/s, around 0.50 seconds even without acceleration. New movement
now needs only 10 cm before its first sound; later contacts keep the existing stride.
Fractional distance is preserved between contacts to prevent frame-rate drift.
Stopping or leaving the ground resets startup; the cooldown prevents rapid
stop/start double hits. Landing counts as the initial contact and retains its guard.
Bank loading primes the short footstep/landing sounds before playback.

The source wave onset is 1.5–14 ms for dirt and 0.8–1.2 ms for metal, so leading
silence in those files does not explain the roughly half-second initial delay.
Regression cases cover slow acceleration, restart, short taps, landing, frame rates,
airborne movement and hitches. The modified native tests and gameplay timing still
need the user's Rider build and an interactive walk check. Audio is still driven
by distance cadence, not animation contact notifies; exact foot-to-ground alignment
is not claimed.

## Build and checks

### Planetary structure footsteps — source prepared for Rider

The landing pad and procedural access ramp have no authored physical surface.
Previously only ship/station actors (or their gravity sources) selected metal,
so planetary structures fell back to dirt. The contact resolver now recognizes
the actual supporting pad, base, colony/module, headquarters, ship or station.
It resolves each contact separately, so leaving a deck restores terrain sounds.
The five metal variants keep their existing volume and repetition guard.
Landing on a deck also uses its contact set instead of the dirt landing cue.

An explicitly mapped physical surface takes precedence. If a movement floor hit
omits its physical material, the resolver reads the contacted body's material.
This does not change collision, friction, gravity or movement. Generic default
surface mappings cannot hide a known deck; empty sets fall back safely.

Live read-only inspection confirmed the generated pad's Deck/AccessRamp have no
physical material override, the bank's surface map is empty and all five metal
cues exist. The source regression test APS.Audio.FootstepSurfaces covers ground,
deck, both ramp types, leaving the deck, colony/ship/station floors and material
overrides. It has not been run against the new code: build in Rider, restart the
editor, run the test, then listen while walking ground -> ramp -> deck -> ground
and landing on the deck. Existing startup cadence and user mix levels are unchanged.

Build APS_ALPHAEditor / Development Editor / Win64 in Rider, then restart the editor
to load the new module. Test slow walk (mode 1), stop/start, run, jump and landing.
Run APS.Audio.FootstepCadence and APS.Audio.EngineEnvelopeAndVolumes.

For bank validation in a free editor/test window, use Tools/Audio/Initialize-APSAudio.ps1.
It checks an existing bank without overwriting tuned assets. The commandlet source
now references the melodic sequence; its compiled recipe changes after the Rider build.
A fresh bank creation requires the imported melodic wave and vehicle recordings already present.

Earlier baseline checks: bank creation produced 34 assets / 25 cues / 5 classes;
both original APS.Audio tests passed. That earlier result does not validate the new
footstep code. Logs: Saved/Logs/APSAudio-20261002-052706.log and
Saved/Logs/APSAudio-RuntimeVerify2.log.

Before release, also verify sliders/persistence, UI actions, engine transitions,
zero-G silence for steps, level changes and packaged audio dependencies.

## Integration

### Ground vehicles

Rover, hover and drone still use ASpaceship for movement. The audio subsystem
branches on GetGroundVehicleKind before the spacecraft mix: each vehicle has its
own idle/drive recordings, and the rover has a recorded starter. Ground vehicles
do not play spacecraft start/stop, band-change or warp sounds.

The mix observes pilot input, engine power and kinematic speed. Gas is audible
even against an obstacle, coasting lowers load, reverse loads the rover, and
drone climb/strafe/descent change rotor load. Holding boost without thrust does
not rev the motor. Hover lift and drone rotors remain audible while powered at
rest. Existing volume/pitch smoothing handles changes; engine-off, leaving the
seat, changing pawn and world teardown release the loops. All use Effects volume.
This is the controlled vehicle's local mix; parked NPC vehicles have no new emitters.

Seven SoundWaves live in /Game/APS/APS_ALPHA/Audio/Vehicles. They load asynchronously
in a separate request, are retained by the subsystem and primed before use.
The existing Audio cook directory includes them; the audio bank and its tuned
music references require no migration. No new plugin is required.

Authoring: SourceAudio/APS/Vehicles contains rendered PCM and VehicleAudio.json.
Originals/ contains the five recordings used and their source/license manifest.
Tools/Audio/Render-VehicleAudio.py reproduces the edits with Python/numpy.
Tools/Audio/Import-VehicleAudio.py imports only missing vehicle assets; existing
assets are checked and never silently overwritten. VehicleAudio.txt ships via
the existing AudioCredits NonUFS directory.

Signal checks passed for all seven renders: finite 48 kHz mono PCM, bounded
levels, short loop-wrap transitions, matched hashes and no clipping. Source RMS
targets are -23 to -28 dBFS with peaks below -10 dBFS, before the gameplay mix.
This does not establish listening acceptance. After the user's Rider build and
editor restart, run APS.Audio.GroundVehicleProfiles and APSAudioAsset -Validate,
then listen to each vehicle at idle, under gas, reverse/braking, boost, engine
toggle, exit, and drone vertical flight. Check switching back to a spacecraft.

- UAPSAudioSubsystem observes the local Game/PIE pawn; teardown stops owned sounds.
- The curated UAPSAudioBank holds cues, music, surfaces and mix routing.
- Sound classes: Master -> Music, Ambience, Effects, UI.
- User preferences live in Saved/Config/<platform>/APSAudioUser.ini.
- Footsteps use tangential velocity; floating-origin shifts cannot cause contacts.
- Dirt and metal have five variants each; physical-surface overrides are supported.
- Engine load observes speed, acceleration, boost/brake and flight band.
- Cooking includes the bank folder and referenced dependencies.

## Sources

| Use | Source |
| --- | --- |
| Click / hover / back | Interface & Item Sounds Pack |
| Confirm / flight mode | Energy Fields SFX Pack |
| Menu | Scott Buckley: A Kind Of Hope, Tears in Rain, Celestial |
| Exploration | Scott Buckley: A Kind Of Hope, Tears in Rain, Celestial |
| Engines | Energy Fields SFX Pack |
| Engine start / stop | 237 SCI-FI Game Sound Effects |
| Terrain / metal steps | Footsteps Mini Sound Pack |

Space Ambient Soundscapes Bundle remains installed, with no active default background
references in the bank. Full credits are in THIRD_PARTY_AUDIO.txt; required footsteps
attribution is also shown in Audio settings.

NPC/distant-ship positional audio and dedicated planet weather remain outside this layer.

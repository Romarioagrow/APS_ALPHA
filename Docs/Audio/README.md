# APOSFERA audio

## Current state — 2026-10-02

The user confirmed audible menu and gameplay sounds. The bank is installed under
`/Game/APS/APS_ALPHA/Audio`. The following background revision is saved and active
in the running editor; it does not require a C++ build.

### Main generation menu — three musical themes

The user rejected the NewAge menu track and requested all three directions:
piano/strings, melodic electronica, and calm orchestral music. Only SC_Music_Menu
was replaced in this revision. Its source is now SW_Menu_MelodicPlaylist:

1. A Kind Of Hope — Scott Buckley; piano and strings. Starts at 0:00.
2. Tears in Rain — Scott Buckley; cinematic electronica. Starts at about 5:37.
3. Celestial — Scott Buckley; orchestral. Starts at about 11:11.

All three complete compositions play in a fixed cycle of 1007.85 seconds (16:48).
Six-second fades overlap the transitions. The cycle ends and starts at silence;
there is no abrupt cut at the repeat. This is an authored streamed audio sequence,
not a runtime shuffle. Returning to the menu starts from the first composition.

Each original was measured in LUFS and attenuated to approximately -28 LUFS before
the common cue gain of 0.55 (about -33.2 LUFS afterward at unity user settings).
Natural dynamics are retained; the rendered sequence peak is -13.49 dBFS before
the cue gain. These checks establish levels and transitions, not subjective approval.

The music asset and bank are saved through the current editor. Playback needs no
module rebuild. The source recipe and the Audio settings credit line take effect
after the user's next Rider build. Credits are also in Content/AudioCredits/MenuMusic.txt;
the packaging config includes that directory as loose staged files.

Original MP3s, source links/hashes and the authoring WAV are kept in
SourceAudio/APS/MenuMusic. Reproduce with Python/numpy and FFmpeg:

```powershell
python Tools/Audio/Render-MenuMusic.py SourceAudio/APS/MenuMusic/Originals output_directory path/to/ffmpeg.exe
```

Import output_directory/SW_Menu_MelodicPlaylist.wav into
/Game/APS/APS_ALPHA/Audio/Waves/SW_Menu_MelodicPlaylist.
All three tracks are from the composer's CC BY 4.0 library; see THIRD_PARTY_AUDIO.txt.

### Gameplay background

The live inspection found SC_Music_Exploration (Mind) as the only playing component
while the character stood on the surface. That was the reported continuous hum.
It has been replaced by SW_Exploration_CalmSuite, a 224-second edit of the licensed
NewAge track. Four different 52-second passages play in this order: source offsets
4, 128, 66 and 190 seconds. Each has a 7-second fade-in, a 9-second fade-out and a
4-second pause afterward. The sequence repeats after all four passages; its order
is fixed, not randomized. Both ends are silent, so the wrap has no waveform jump.

A gentle 70 Hz high-pass reduces sub-bass and a 3200 Hz low-pass softens the top end.
Passages are attenuated as needed but never amplified. Source RMS is -31.35 dBFS,
peak -14.27 dBFS; cue gain is 0.65. Its average cue output is approximately 9.3 dB
below the former Mind cue before user volume settings. These are measurements,
not subjective listening approval.

SpaceAmbience and InteriorAmbience are unassigned in the bank, avoiding additional
continuous drones over the musical background. Their old cue assets remain available
for later deliberate sound design. Engine sounds, steps, UI and user volume settings
are unchanged by the background edit. The separate menu cue uses the three-theme sequence described above.

Authoring files: `SourceAudio/APS/SW_Exploration_CalmSuite.wav` and
`SourceAudio/APS/CalmExploration.json`. To recreate, export NewAge_wav to stereo
16-bit PCM using Unreal's SoundExporterWAV, then run (Python with numpy):

```powershell
python Tools/Audio/Render-CalmExploration.py NewAge.wav SourceAudio/APS
```

Reimport into `/Game/APS/APS_ALPHA/Audio/Waves/SW_Exploration_CalmSuite`.
The renderer does not modify the source marketplace asset.

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

Build APS_ALPHAEditor / Development Editor / Win64 in Rider, then restart the editor
to load the new module. Test slow walk (mode 1), stop/start, run, jump and landing.
Run APS.Audio.FootstepCadence and APS.Audio.EngineEnvelopeAndVolumes.

For bank validation in a free editor/test window, use Tools/Audio/Initialize-APSAudio.ps1.
It checks an existing bank without overwriting tuned assets. The commandlet source
now references the calm suite; its compiled recipe changes after the Rider build.
A fresh bank creation requires the imported calm-suite wave already present.

Earlier baseline checks: bank creation produced 34 assets / 25 cues / 5 classes;
both original APS.Audio tests passed. That earlier result does not validate the new
footstep code. Logs: Saved/Logs/APSAudio-20261002-052706.log and
Saved/Logs/APSAudio-RuntimeVerify2.log.

Before release, also verify sliders/persistence, UI actions, engine transitions,
zero-G silence for steps, level changes and packaged audio dependencies.

## Integration

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
| Exploration | Mix Of Ambient Music: calm four-part edit of NewAge |
| Engines | Energy Fields SFX Pack |
| Engine start / stop | 237 SCI-FI Game Sound Effects |
| Terrain / metal steps | Footsteps Mini Sound Pack |

Space Ambient Soundscapes Bundle remains installed, with no active default background
references in the bank. Full credits are in THIRD_PARTY_AUDIO.txt; required footsteps
attribution is also shown in Audio settings.

NPC/distant-ship positional audio and dedicated planet weather remain outside this layer.

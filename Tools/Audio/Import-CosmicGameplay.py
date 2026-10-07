"""Run in the existing Unreal editor after installing SourceAudio/APS/CosmicGameplay.
Imports a new gameplay wave and updates only SC_Music_Exploration. Saves backups.
"""
import unreal,pathlib,json,hashlib,datetime,shutil
project=pathlib.Path(unreal.Paths.project_dir())
source=project/'SourceAudio/APS/CosmicGameplay'
meta=json.loads((source/'CosmicJourney.json').read_text(encoding='utf-8'))
wav=source/'SW_Gameplay_CosmicJourney_v1.wav'
assert hashlib.sha256(wav.read_bytes()).hexdigest()==meta['sha256']
root='/Game/APS/APS_ALPHA/Audio/'
name='SW_Gameplay_CosmicJourney_v1'
cue=unreal.load_asset(root+'SC_Music_Exploration')
bank=unreal.load_asset(root+'DA_APSAudioBank')
assert cue and bank.get_editor_property('exploration_music')==cue
node=cue.get_editor_property('first_node')
assert isinstance(node,unreal.SoundNodeWavePlayer)
existing=node.get_editor_property('sound_wave_asset_ptr')
if existing and existing.get_name()==name:
    assert abs(existing.get_editor_property('duration')-meta['duration_s'])<.2
    assert abs(cue.get_editor_property('volume_multiplier')-meta['gameplay_gain'])<.001
    print('Cosmic gameplay music already installed; no assets changed.')
else:
    targets=[root+'SC_Music_Exploration',root+'Waves/'+name]
    dirty={p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert not any(p in dirty for p in targets),'Unsaved user edits on target audio assets'
    assert not (project/'Content/APS/APS_ALPHA/Audio/Waves'/(name+'.uasset')).exists(),'Unexpected existing wave; inspect before overwriting'
    preserved=['DA_APSAudioBank.uasset','SC_Music_Menu.uasset','SCL_Music.uasset','Waves/SW_MelodicMusic_v2.uasset']
    content=project/'Content/APS/APS_ALPHA/Audio'
    original_hashes={p:hashlib.sha256((content/p).read_bytes()).hexdigest() for p in preserved}
    backup=project/'Saved/AudioIntegrationBackups'/('cosmic-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    backup.mkdir(parents=True)
    shutil.copy2(content/'SC_Music_Exploration.uasset',backup/'SC_Music_Exploration.uasset')
    task=unreal.AssetImportTask()
    task.filename=str(wav);task.destination_path=root+'Waves';task.destination_name=name
    task.automated=True;task.replace_existing=False;task.save=False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=unreal.load_asset(root+'Waves/'+name)
    assert isinstance(sound,unreal.SoundWave)
    assert abs(sound.get_editor_property('duration')-meta['duration_s'])<.2
    sound.set_editor_property('sound_class_object',bank.get_editor_property('music_class'))
    sound.set_editor_property('loading_behavior',unreal.SoundWaveLoadingBehavior.RETAIN_ON_LOAD)
    sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    sound.set_editor_property('compression_quality',65)
    node.set_editor_property('sound_wave_asset_ptr',sound)
    node.set_editor_property('looping',True)
    cue.set_editor_property('volume_multiplier',meta['gameplay_gain'])
    cue.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    assert unreal.EditorLoadingAndSavingUtils.save_packages([sound.get_outer(),cue.get_outer()],only_dirty=False)
    unreal.GameplayStatics.prime_sound(cue)
    for path,digest in original_hashes.items():
        assert hashlib.sha256((content/path).read_bytes()).hexdigest()==digest,'Unexpected change: '+path
    # Ask only the currently playing gameplay music to release its old recording.
    # The existing audio subsystem recreates this auto-destroyed component.
    faded=[]
    for component in unreal.ObjectIterator(unreal.AudioComponent):
        if component.is_playing() and component.get_editor_property('sound')==cue:
            component.fade_out(1.2,0.0);faded.append(component.get_path_name())
    result={'wave':sound.get_path_name(),'previous_wave':existing.get_path_name() if existing else None,
            'duration_s':sound.get_editor_property('duration'),'tracks':len(meta['tracks']),
            'cue_gain':meta['gameplay_gain'],'preserved_hashes':original_hashes,
            'backup':str(backup),'faded_gameplay_components':faded}
    (backup/'cosmic-change.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result))

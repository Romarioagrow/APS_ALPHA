"""Run in Unreal Python after rendering. Imports/validates only the vehicle sounds."""
import hashlib,json,pathlib,unreal

project=pathlib.Path(unreal.Paths.project_dir())
source_dir=project/'SourceAudio'/'APS'/'Vehicles'
meta=json.loads((source_dir/'VehicleAudio.json').read_text(encoding='utf-8'))
root='/Game/APS/APS_ALPHA/Audio/Vehicles'
effects=unreal.load_asset('/Game/APS/APS_ALPHA/Audio/SCL_Effects')
if not effects: raise RuntimeError('Existing APS Effects class is required')
rows=[]
for row in meta['sounds']:
    source=source_dir/(row['name']+'.wav')
    if hashlib.sha256(source.read_bytes()).hexdigest()!=row['sha256']:
        raise RuntimeError('Rendered source changed: '+str(source))
    asset_path=root+'/'+row['name']
    asset_file=project/'Content'/'APS'/'APS_ALPHA'/'Audio'/'Vehicles'/(row['name']+'.uasset')
    exists=asset_file.exists()
    # The asset may already be loaded but unsaved after an interrupted import.
    sound=unreal.find_object(None,asset_path+'.'+row['name'])
    if not exists and sound is None:
        task=unreal.AssetImportTask()
        task.filename=str(source)
        task.destination_path=root
        task.destination_name=row['name']
        task.automated=True
        task.replace_existing=False
        task.save=False
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=unreal.load_asset(asset_path)
    if not isinstance(sound,unreal.SoundWave): raise RuntimeError('Expected SoundWave: '+asset_path)
    if abs(sound.get_editor_property('duration')-row['duration_s'])>.02: raise RuntimeError('Duration mismatch: '+asset_path)
    if not exists:
        sound.set_editor_property('looping',row['looping'])
        sound.set_editor_property('sound_class_object',effects)
        sound.set_editor_property('volume',1.0)
        sound.set_editor_property('pitch',1.0)
        sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT if row['looping'] else unreal.VirtualizationMode.DISABLED)
        sound.set_editor_property('loading_behavior',unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
        sound.set_sound_asset_compression_type(unreal.SoundAssetCompressionType.PCM)
        if not unreal.EditorLoadingAndSavingUtils.save_packages([sound.get_outer()],only_dirty=False):
            raise RuntimeError('Save failed: '+asset_path)
    assert sound.get_editor_property('looping')==row['looping'], 'Existing loop metadata differs: '+asset_path
    assert sound.get_editor_property('num_channels')==1
    assert sound.get_editor_property('sound_class_object')==effects
    assert sound.get_editor_property('loading_behavior')==unreal.SoundWaveLoadingBehavior.FORCE_INLINE
    rows.append({'asset':asset_path,'duration_s':sound.get_editor_property('duration'),'looping':row['looping'],'created':not exists})
print(json.dumps({'vehicle_sounds':rows,'count':len(rows)}))

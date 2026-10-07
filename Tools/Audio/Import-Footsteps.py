"""Run in the existing editor. Update only the owned footstep bank and cues."""
import unreal,pathlib,json,hashlib,datetime,shutil
project=pathlib.Path(unreal.Paths.project_dir())
source=project/'SourceAudio/APS/Footsteps'
meta=json.loads((source/'Footsteps.json').read_text(encoding='utf-8'))
root='/Game/APS/APS_ALPHA/Audio'
loaded_waves={w.get_name():w for w in unreal.ObjectIterator(unreal.SoundWave) if w.get_path_name().startswith(root+'/Footsteps/')}
def find_wave(name):
    if name not in loaded_waves and (project/'Content/APS/APS_ALPHA/Audio/Footsteps'/(name+'.uasset')).exists():
        loaded_waves[name]=unreal.load_asset(root+'/Footsteps/'+name)
    return loaded_waves.get(name)
bank=unreal.load_asset(root+'/DA_APSAudioBank')
effects=bank.get_editor_property('effects_class')
old_cues=[unreal.load_asset(root+f'/SC_Step_Metal_{i:02}') for i in range(1,6)]
for cue in old_cues: assert isinstance(cue.get_editor_property('first_node'),unreal.SoundNodeWavePlayer)
targets={root+'/DA_APSAudioBank',*[root+f'/SC_Step_Metal_{i:02}' for i in range(1,6)]}
ours=set()
for cue in old_cues:
    w=cue.get_editor_property('first_node').get_editor_property('sound_wave_asset_ptr')
    expected=root+'/Footsteps/SW_Step_Metal_'+cue.get_name()[-2:]
    if w and w.get_path_name()==expected+'.'+expected.rsplit('/',1)[1] and abs(cue.get_editor_property('volume_multiplier')-1)<.001:
        ours.add(cue.get_outer().get_name())
for row in meta['waves']:
    assert hashlib.sha256((source/(row['name']+'.wav')).read_bytes()).hexdigest()==row['sha256']
    targets.add(root+'/Footsteps/'+row['name'])
    existing=find_wave(row['name'])
    if existing:
        imported=existing.get_editor_property('asset_import_data').get_first_filename()
        assert pathlib.Path(imported).resolve()==(source/(row['name']+'.wav')).resolve(), 'Unexpected source: '+imported
        assert abs(existing.get_editor_property('duration')-row['duration_s'])<.002
        ours.add(existing.get_outer().get_name())
assert not (targets-ours).intersection({p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}),'Unsaved target edits'
fields=['mix','master_class','music_class','ambience_class','effects_class','ui_class','ui_click','ui_hover','ui_confirm','ui_back',
    'menu_music','exploration_music','space_ambience','interior_ambience','ship_idle_loop','ship_thrust_loop','ship_warp_loop','engine_start','engine_stop','flight_mode_change','landing']
def refs(): return {p:(bank.get_editor_property(p).get_path_name() if bank.get_editor_property(p) else None) for p in fields}
preserve=refs()
backup=project/'Saved/AudioIntegrationBackups'/('footsteps-v2-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True)
content=project/'Content/APS/APS_ALPHA/Audio'
for name in ['DA_APSAudioBank']+[c.get_name() for c in old_cues]: shutil.copy2(content/(name+'.uasset'),backup/(name+'.uasset'))
tools=unreal.AssetToolsHelpers.get_asset_tools()
packages=[];sets={};report=[]
for row in meta['waves']:
    name=row['name'];surface=row['surface']
    if not find_wave(name):
        task=unreal.AssetImportTask();task.filename=str(source/(name+'.wav'));task.destination_path=root+'/Footsteps';task.destination_name=name
        task.automated=True;task.replace_existing=False;task.save=False
        tools.import_asset_tasks([task])
    sound=unreal.load_asset(root+'/Footsteps/'+name)
    assert isinstance(sound,unreal.SoundWave)
    sound.set_editor_property('sound_class_object',effects)
    sound.set_editor_property('loading_behavior',unreal.SoundWaveLoadingBehavior.FORCE_INLINE)
    sound.set_editor_property('compression_quality',80)
    sound.set_editor_property('looping',False)
    playback=sound
    if surface=='Metal':
        playback=old_cues[int(name[-2:])-1]
        node=playback.get_editor_property('first_node');node.set_editor_property('sound_wave_asset_ptr',sound);node.set_editor_property('looping',False)
        playback.set_editor_property('volume_multiplier',1.0);playback.set_editor_property('sound_class_object',effects)
        packages.append(playback.get_outer())
    sets.setdefault(surface,[]).append(playback)
    packages.append(sound.get_outer())
    report.append({'playback':playback.get_path_name(),'wave':sound.get_path_name(),'duration':sound.get_editor_property('duration')})
mapping=dict(bank.get_editor_property('surface_footsteps'))
# Native enum wrappers are generated on editor startup; a running editor may
# predate the six names in DefaultEngine.ini. BankLoaded registers these sets
# from the same assets after the next Rider build, without another import.
surface_keys={}
for n in range(1,7):
    try: surface_keys[n]=unreal.PhysicalSurface.cast(n)
    except TypeError: pass
if 1 in surface_keys: mapping[surface_keys[1]]=bank.get_editor_property('default_footsteps')
for n,surface in enumerate(['Metal','Plastic','Stone','Wood','Snow'],2):
    entry=unreal.APSAudioFootsteps();entry.set_editor_property('sounds',sets[surface])
    if n in surface_keys: mapping[surface_keys[n]]=entry
    if surface=='Metal': bank.set_editor_property('metal_footsteps',entry)
bank.set_editor_property('surface_footsteps',mapping)
assert refs()==preserve,'Unrelated audio bank fields changed'
packages.append(bank.get_outer())
assert unreal.EditorLoadingAndSavingUtils.save_packages(packages,only_dirty=False)
for row in report:
    sound=unreal.load_asset(row['wave']);assert not sound.get_editor_property('looping')
    unreal.GameplayStatics.prime_sound(sound)
report={'sounds':report,'sets':{s:len(v) for s,v in sets.items()},'preserved_bank_fields':preserve,'backup':str(backup),
    'saved_surface_keys':list(surface_keys),'runtime_registration_after_build':True}
(backup/'footsteps-change.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'saved':len(packages),'sets':report['sets'],'backup':str(backup)}))

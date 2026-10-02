"""Author three complete tracks into a streamed menu sequence.

Usage: python render_menu_playlist.py downloads output_directory /path/to/ffmpeg.exe
The supplied sources.json records the licensed originals. Requires numpy.
"""
import pathlib,sys,subprocess,json,wave,hashlib,math
import numpy as np
source_dir,out=map(pathlib.Path,sys.argv[1:3])
ffmpeg=sys.argv[3]
out.mkdir(parents=True,exist_ok=True)
tracks=json.loads((source_dir/'sources.json').read_text(encoding='utf-8-sig'))
rate=44100
audio=[]
report=[]
for index,track in enumerate(tracks):
    source=source_dir/track['filename']
    if hashlib.sha256(source.read_bytes()).hexdigest()!=track['sha256'].lower(): raise RuntimeError('Source hash mismatch')
    measured=subprocess.run([ffmpeg,'-hide_banner','-nostdin','-threads','2','-i',str(source),'-af','loudnorm=I=-28:TP=-9:LRA=11:print_format=json','-f','null','-'],capture_output=True,text=True,check=True)
    meter=json.JSONDecoder().raw_decode(measured.stderr[measured.stderr.rfind('{'):])[0]
    # Match integrated loudness while keeping natural dynamics. Only attenuate,
    # and cap the original true peak at -12 dBFS before the common cue gain.
    gain_db=min(0.,-28-float(meter['input_i']),-12-float(meter['input_tp']))
    decoded=subprocess.run([ffmpeg,'-v','error','-nostdin','-threads','2','-i',str(source),'-f','f32le','-ac','2','-ar',str(rate),'pipe:1'],capture_output=True,check=True)
    x=np.frombuffer(decoded.stdout,dtype='<f4').reshape(-1,2).copy()
    del decoded
    x*=10**(gain_db/20)
    # Preserve each complete composition, including its own ending. Extra gentle
    # fades only cover the first/last six seconds, and keep every transition quiet.
    fade=rate*6
    x[:fade]*=(np.sin(np.linspace(0,np.pi/2,fade,dtype=np.float32))**2)[:,None]
    x[-fade:]*=(np.cos(np.linspace(0,np.pi/2,fade,dtype=np.float32))**2)[:,None]
    track_name='SW_Menu_'+['AKindOfHope','TearsInRain','Celestial'][index]
    with wave.open(str(out/(track_name+'.wav')),'wb') as w:
        w.setparams((2,2,rate,0,'NONE','not compressed'))
        w.writeframes(np.round(x*32767).astype('<i2').tobytes())
    audio.append(x)
    row=dict(track,duration_s=len(x)/rate,input_lufs=float(meter['input_i']),input_true_peak_dbfs=float(meter['input_tp']),gain_db=gain_db,wave=track_name)
    report.append(row)
    print(json.dumps(row),flush=True)

crossfade=6*rate
total=sum(len(x) for x in audio)-crossfade*2
suite=np.zeros((total,2),dtype=np.float32)
cursor=0
for i,x in enumerate(audio):
    report[i]['playlist_start_s']=cursor/rate
    # The per-track cosine fade-out and sine fade-in provide a gentle overlap.
    suite[cursor:cursor+len(x)]+=x
    cursor+=len(x)-crossfade if i<2 else len(x)
assert cursor==len(suite)
assert np.isfinite(suite).all() and np.max(np.abs(suite))<1
output=out/'SW_Menu_MelodicPlaylist.wav'
with wave.open(str(output),'wb') as w:
    w.setparams((2,2,rate,0,'NONE','not compressed'))
    # Bound the final quantization allocation for long sequences.
    for start in range(0,len(suite),rate*30):
        w.writeframes(np.round(suite[start:start+rate*30]*32767).astype('<i2').tobytes())
meta={'tracks':report,'duration_s':len(suite)/rate,'sample_rate':rate,'crossfade_s':6,'cue_gain':0.55,'order':'fixed; all three complete tracks before repeating','sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'peak_dbfs':float(20*np.log10(np.max(np.abs(suite))))}
(out/'MenuMusic.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in meta.items() if k!='tracks'}),flush=True)

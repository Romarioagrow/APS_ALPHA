"""Rebuild the accepted three tracks with audible starts and bounded dynamics."""
import pathlib,subprocess,json,wave,hashlib,sys
import numpy as np
source=pathlib.Path(sys.argv[1])
output_directory=pathlib.Path(sys.argv[2])
ffmpeg=sys.argv[3]
tracks=json.loads((source/'sources.json').read_text(encoding='utf-8-sig'))
out=output_directory
out.mkdir(exist_ok=True)
rate=44100
rendered=[]
rows=[]
for track in tracks:
    path=source/track['filename']
    assert hashlib.sha256(path.read_bytes()).hexdigest()==track['sha256'].lower()
    # Dynamic loudness normalization lifts quiet passages without driving crescendos into clipping.
    args=[ffmpeg,'-hide_banner','-nostdin','-threads','2','-i',str(path),
        '-af','highpass=f=30,loudnorm=I=-23:TP=-9:LRA=8:linear=false:print_format=json',
        '-f','f32le','-ac','2','-ar',str(rate),'pipe:1']
    run=subprocess.run(args,capture_output=True,check=True)
    x=np.frombuffer(run.stdout,'<f4').reshape(-1,2).copy()
    meter=json.JSONDecoder().raw_decode(run.stderr.decode()[run.stderr.decode().rfind('{'):])[0]
    # Trim only the barely audible lead-in before starting the menu's short fade.
    hop=rate//10
    blocks=np.sqrt(np.mean(x[:len(x)//hop*hop].reshape(-1,hop,2)**2,axis=(1,2)))
    audible=np.flatnonzero(blocks>10**(-40/20))
    trim=max(0,(int(audible[0])-1)*hop) if len(audible) else 0
    x=x[trim:]
    fade_in=int(rate*.6)
    fade_out=rate*4
    x[:fade_in]*=np.linspace(0,1,fade_in,dtype=np.float32)[:,None]
    x[-fade_out:]*=np.linspace(1,0,fade_out,dtype=np.float32)[:,None]
    rendered.append(x)
    rows.append(dict(track,trim_start_s=trim/rate,duration_s=len(x)/rate,normalization=meter))
    print(json.dumps({'title':track['title'],'trim_s':trim/rate,'seconds':len(x)/rate,'output_lufs':meter.get('output_i')}),flush=True)
overlap=rate*4
playlist=np.zeros((sum(len(x) for x in rendered)-overlap*2,2),np.float32)
cursor=0
for i,x in enumerate(rendered):
    rows[i]['playlist_start_s']=cursor/rate
    if i: x[:overlap]*=np.linspace(0,1,overlap,dtype=np.float32)[:,None]
    playlist[cursor:cursor+len(x)]+=x
    cursor+=len(x)-overlap if i<2 else len(x)
peak=float(np.max(abs(playlist)))
assert np.isfinite(playlist).all() and peak<.4
name='SW_MelodicMusic_v2'
path=out/(name+'.wav')
with wave.open(str(path),'wb') as f:
    f.setparams((2,2,rate,0,'NONE','not compressed'))
    for start in range(0,len(playlist),rate*30):
        f.writeframes(np.round(playlist[start:start+rate*30]*32767).astype('<i2').tobytes())
meta={'name':name,'tracks':rows,'sample_rate':rate,'duration_s':len(playlist)/rate,
    'menu_gain':.67,'gameplay_gain':.52,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
    'peak_dbfs':float(20*np.log10(peak)),'crossfade_s':4,'entry_fade_s':.6}
(out/'MelodicMusic.json').write_text(json.dumps(meta,indent=2))
with wave.open(str(out/'MusicPreview.wav'),'wb') as f:
    f.setparams((2,2,rate,0,'NONE','not compressed'))
    f.writeframes(np.round(playlist[:rate*20]*.67*32767).astype('<i2').tobytes())
print(json.dumps({k:v for k,v in meta.items() if k!='tracks'}),flush=True)

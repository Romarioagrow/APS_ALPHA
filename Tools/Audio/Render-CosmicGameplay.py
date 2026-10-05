"""Render eight licensed electronic compositions as a streamed gameplay cycle.

Usage: python Render-CosmicGameplay.py originals_directory output_directory ffmpeg.exe
Requires numpy. Does not download assets or alter the originals.
"""
import hashlib,json,math,pathlib,subprocess,sys,wave
import numpy as np

source,output=map(pathlib.Path,sys.argv[1:3]); ffmpeg=sys.argv[3]
output.mkdir(parents=True,exist_ok=True)
tracks=json.loads((source/'sources.json').read_text(encoding='utf-8'))
assert len(tracks)==8 and len({t['title'] for t in tracks})==8
rate=44100; overlap=6.0; game_gain=0.65
eq='highpass=f=28,highshelf=f=3500:g=-2'

def run(args):
    result=subprocess.run([ffmpeg,'-hide_banner','-nostdin','-threads','2',*args],capture_output=True)
    if result.returncode:
        print(result.stderr.decode('utf-8',errors='replace')[-5000:],file=sys.stderr)
        result.check_returncode()
    return result

def write_wav(path,data):
    assert np.isfinite(data).all() and np.max(np.abs(data))<1.0
    with wave.open(str(path),'wb') as f:
        f.setparams((2,2,rate,0,'NONE','not compressed'))
        # Work in chunks to avoid an extra full-playlist PCM allocation.
        for begin in range(0,len(data),rate*10):
            f.writeframesraw(np.rint(data[begin:begin+rate*10]*32767).astype('<i2').tobytes())

rows=[];paths=[];preview=[]
for index,t in enumerate(tracks):
    path=source/t['filename'];assert hashlib.sha256(path.read_bytes()).hexdigest()==t['sha256']
    measured=run(['-i',str(path),'-af',eq+',loudnorm=I=-24:TP=-9:LRA=20:print_format=json','-f','null','-'])
    log=measured.stderr.decode('utf-8',errors='replace')
    meter=json.JSONDecoder().raw_decode(log[log.rfind('{'):])[0]
    decoded=run(['-i',str(path),'-af',eq,'-f','f32le','-ac','2','-ar',str(rate),'pipe:1'])
    x=np.frombuffer(decoded.stdout,'<f4').reshape(-1,2).copy()
    del decoded
    gain_db=min(-24.0-float(meter['input_i']),-8.0-float(meter['input_tp']))
    x*=10**(gain_db/20)
    # The album intros can stay nearly inaudible for a minute. These gameplay
    # edits enter at audible music and crossfade before the long quiet tails.
    # A two-second RMS window ignores isolated early transients.
    hop=rate//4
    power=np.mean(x[:len(x)//hop*hop].reshape(-1,hop,2)**2,axis=(1,2))
    energy=np.sqrt(np.convolve(power,np.ones(8)/8,'valid'))
    audible=np.flatnonzero(energy>10**(-38/20))
    assert len(audible),t['title']
    start=max(0,(int(audible[0])-2)*hop)
    end=min(len(x),(int(audible[-1])+8+2)*hop)
    original_duration=len(x)/rate
    x=x[start:end]
    fade=int(rate*0.8)
    x[:fade]*=np.linspace(0,1,fade,dtype=np.float32)[:,None]
    x[-fade:]*=np.linspace(1,0,fade,dtype=np.float32)[:,None]
    track_path=output/f'{index+1:02d}_normalized.wav';write_wav(track_path,x);paths.append(track_path)
    clip=x[int(len(x)*.3):int(len(x)*.3)+rate*12].copy()*game_gain
    clip[:rate]*=np.linspace(0,1,rate,dtype=np.float32)[:,None]
    clip[-rate:]*=np.linspace(1,0,rate,dtype=np.float32)[:,None]
    preview.append(clip)
    row=dict(t,trim_start_s=start/rate,trim_end_s=original_duration-end/rate,
             original_duration_s=original_duration,duration_s=len(x)/rate,gain_db=gain_db,
             input_lufs=float(meter['input_i']),estimated_output_lufs=float(meter['input_i'])+gain_db,
             peak_dbfs=20*math.log10(float(np.max(np.abs(x)))))
    rows.append(row);print(json.dumps({k:row[k] for k in ['title','duration_s','gain_db','estimated_output_lufs','peak_dbfs']}),flush=True)

args=[]
for p in paths:args+=['-i',str(p)]
filters=[];prior='0:a'
for i in range(1,len(paths)):
    label=f'm{i}'
    filters.append(f'[{prior}][{i}:a]acrossfade=d={overlap}:c1=tri:c2=tri[{label}]');prior=label
final=output/'SW_Gameplay_CosmicJourney_v1.wav'
run(['-y',*args,'-filter_complex_threads','1','-filter_complex',';'.join(filters),'-map',f'[{prior}]',
     '-ar',str(rate),'-ac','2','-c:a','pcm_s16le',str(final)])
with wave.open(str(final),'rb') as f:
    duration=f.getnframes()/f.getframerate(); peak=0; endpoint=[]
    while data:=f.readframes(rate*30):
        pcm=np.frombuffer(data,'<i2').astype(np.float32)/32768
        peak=max(peak,float(np.max(np.abs(pcm))))
    f.rewind();endpoint.append(np.frombuffer(f.readframes(1),'<i2').tolist())
    f.setpos(f.getnframes()-1);endpoint.append(np.frombuffer(f.readframes(1),'<i2').tolist())
assert peak<10**(-7.9/20),peak
assert endpoint==[[0,0],[0,0]],endpoint
start=0.0
for t in rows:
    t['playlist_start_s']=start
    start+=t['duration_s']-overlap
preview_wav=output/'CosmicJourney_Preview.wav';write_wav(preview_wav,np.concatenate(preview))
run(['-y','-i',str(preview_wav),'-c:a','libmp3lame','-b:a','192k',str(output/'CosmicJourney_Preview.mp3')])
report={'tracks':rows,'sample_rate':rate,'duration_s':duration,'crossfade_s':overlap,
        'gameplay_gain':game_gain,'target_lufs_before_cue':-24,'eq':eq,
        'peak_dbfs':20*math.log10(peak),'endpoints_pcm16':endpoint,
        'sha256':hashlib.sha256(final.read_bytes()).hexdigest()}
(output/'CosmicJourney.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'complete':str(final),'duration_s':duration,'peak_dbfs':report['peak_dbfs'],'gain':game_gain}),flush=True)

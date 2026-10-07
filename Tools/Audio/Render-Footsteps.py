"""Render owned FootstepsMiniPack sources; output mono PCM16, 48 kHz."""
import pathlib,sys,subprocess,wave,json,hashlib
import numpy as np

src,out,ffmpeg=pathlib.Path(sys.argv[1]),pathlib.Path(sys.argv[2]),sys.argv[3]
out.mkdir(parents=True,exist_ok=True)
families={'Metal':'LowMetal_Mono','Plastic':'Linoleum_Mono','Stone':'PavementTiles_Mono','Wood':'Parquet_Floor_Mono','Snow':'SnowSteps'}
rows=[]
for surface,family in families.items():
    for i in range(1,6):
        source=src/f'{family}_{i:02}.wav'
        # Short boot-on-deck body; remove sub-bass, ringing mids and sharp highs.
        eq='highpass=f=75,highshelf=f=2400:g=-5,lowpass=f=6500'
        if surface=='Metal': eq+=',equalizer=f=900:t=q:w=1.1:g=-4'
        raw=subprocess.run([ffmpeg,'-v','error','-i',str(source),'-af',eq,'-ac','1','-ar','48000','-f','f32le','pipe:1'],check=True,stdout=subprocess.PIPE).stdout
        x=np.frombuffer(raw,'<f4').astype(np.float64)
        peak=max(abs(x)); active=np.flatnonzero(abs(x)>peak*.01)
        x=x[max(0,int(active[0])-48):min(len(x),int(active[-1])+960)]
        # Keep a 1 ms lead; use a 2 ms attack and 35 ms release to avoid clicks.
        n=min(96,len(x)//2);x[:n]*=np.linspace(0,1,n)
        n=min(1680,len(x)//2);x[-n:]*=np.linspace(1,0,n)
        mask=abs(x)>max(abs(x))*.01
        rms=np.sqrt(np.mean(x[mask]**2)); target=-31 if surface=='Metal' else -32
        gain=min(10**(target/20)/rms,10**(-18/20)/max(abs(x)))
        x*=gain
        pcm=np.round(x*32767).astype('<i2');pcm[0]=pcm[-1]=0
        name=f'SW_Step_{surface}_{i:02}'
        dest=out/(name+'.wav')
        with wave.open(str(dest),'wb') as w:
            w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes(pcm.tobytes())
        rows.append({'name':name,'surface':surface,'source_asset':'/Game/FootstepsMiniPack/SoundWav/'+source.stem,
            'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),
            'duration_s':len(x)/48000,'peak_dbfs':20*np.log10(max(abs(x))),'active_rms_dbfs':20*np.log10(np.sqrt(np.mean(x[mask]**2))),
            'filters':eq,'gain':gain})
manifest={'source_pack':'Footsteps Mini Pack (owned Fab library)','sample_rate':48000,'cue_gain':1.0,'waves':rows}
(out/'Footsteps.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
# Preview uses the in-game nominal gain; no extra normalization on playback.
preview=[]
for surface in families:
    for i in range(1,4):
        with wave.open(str(out/f'SW_Step_{surface}_{i:02}.wav'),'rb') as w: data=np.frombuffer(w.readframes(w.getnframes()),'<i2')
        preview.extend([np.round(data*.72).astype('<i2'),np.zeros(17000,dtype='<i2')])
    preview.append(np.zeros(24000,dtype='<i2'))
with wave.open(str(out/'Footsteps-Preview.wav'),'wb') as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(48000);w.writeframes(np.concatenate(preview).tobytes())
print(json.dumps({'rendered':len(rows),'peak_range':[min(r['peak_dbfs'] for r in rows),max(r['peak_dbfs'] for r in rows)]}))

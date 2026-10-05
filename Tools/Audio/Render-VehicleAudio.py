"""Render the seven vehicle sounds from licensed recordings; never alter the sources."""
import argparse, hashlib, json, pathlib, wave
import numpy as np

RATE = 48000

def read(path, pitch=1.0, start=0.0, seconds=None):
    with wave.open(str(path), 'rb') as f:
        if f.getsampwidth() != 2:
            raise ValueError('Expected 16-bit PCM: ' + str(path))
        rate, channels = f.getframerate(), f.getnchannels()
        x = np.frombuffer(f.readframes(f.getnframes()), '<i2').astype(np.float64).reshape(-1, channels).mean(axis=1) / 32768
    x = x[int(start * rate):int((start + seconds) * rate) if seconds else None]
    positions = np.arange(0, len(x) - 1, rate * pitch / RATE)
    return np.interp(positions, np.arange(len(x)), x)

def loopify(x):
    n = min(int(.06 * RATE), len(x) // 8)
    t = np.linspace(0, 1, n, endpoint=False)
    blend = .5 - .5 * np.cos(np.pi * t)
    return np.concatenate((x[n:-n], x[-n:] * (1 - blend) + x[:n] * blend))

def eq(x, low, high):
    frequencies = np.fft.rfftfreq(len(x), 1 / RATE)
    # Smooth periodic filters also remove DC; no discontinuity at the loop wrap.
    gain = (frequencies / np.maximum(frequencies, low)) ** 2
    gain *= 1 / np.sqrt(1 + (frequencies / high) ** 8)
    return np.fft.irfft(np.fft.rfft(x) * gain, n=len(x))

def save(path, x):
    pcm = np.round(np.clip(x, -1, 1) * 32767).astype('<i2')
    with wave.open(str(path), 'wb') as f:
        f.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        f.writeframes(pcm.tobytes())
    return {'duration_s':len(x)/RATE, 'rms_dbfs':float(20*np.log10(np.sqrt(np.mean(x*x)))),
        'peak_dbfs':float(20*np.log10(np.max(np.abs(x)))), 'wrap_delta':float(abs(x[-1]-x[0])),
        'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sources', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    # name, source, playback pitch, excerpt start/length, highpass, lowpass, RMS, looping
    specs = [
        ('SW_Rover_Idle', 'Car_Loop.wav', .90, 0, None, 35, 1500, -25, True),
        ('SW_Rover_Drive', 'Car_Loop.wav', 1.28, 0, None, 45, 2200, -23, True),
        ('SW_Rover_Start', 'Car_Start.wav', 1, 0, None, 45, 2800, -25, False),
        ('SW_Hover_Idle', 'A_PF_ElectricEnergy_Loop.wav', .88, 4, 8, 45, 950, -28, True),
        ('SW_Hover_Drive', 'A_PF_Moving_Loop.wav', 1, 18, 8, 100, 1700, -26, True),
        ('SW_Drone_Idle', 'A_PF_MovingAviaBlades_Loop.wav', .72, 0, None, 70, 1900, -28, True),
        ('SW_Drone_Drive', 'A_PF_MovingAviaBlades_Loop.wav', .94, 0, None, 100, 2400, -26, True)]
    rows = []
    waves = {}
    for name, source, pitch, start, duration, low, high, rms, looping in specs:
        x = read(args.sources/source, pitch, start, duration)
        if looping: x = loopify(x)
        x = eq(x, low, high)
        if not looping:
            n = int(.02*RATE)
            x[:n] *= np.linspace(0, 1, n)
            x[-n:] *= np.linspace(1, 0, n)
        x *= 10**(rms/20) / max(np.sqrt(np.mean(x*x)), 1e-9)
        x *= min(1.0, 10**(-10/20)/max(np.max(np.abs(x)),1e-9))
        assert np.isfinite(x).all() and np.max(np.abs(x)) < .317
        path = args.output/(name+'.wav')
        row = {'name':name, 'source':source, 'source_sha256':hashlib.sha256((args.sources/source).read_bytes()).hexdigest(),
            'looping':looping, 'pitch':pitch, 'excerpt_start_s':start, 'excerpt_s':duration,
            'highpass_hz':low, 'lowpass_hz':high, **save(path,x)}
        if looping: assert row['wrap_delta'] < .015, row
        rows.append(row)
        waves[name] = x
    # Preview: each vehicle idles, accelerates, then winds down. Quiet gaps separate the three.
    preview = []
    for vehicle in ['Rover', 'Hover', 'Drone']:
        count = RATE * 8
        t = np.arange(count)/RATE
        demand = np.interp(t,[0,2,5,6,8],[0,0,1,1,0])
        pos = np.cumsum(.85+.35*demand)
        idle = np.interp(pos%len(waves['SW_'+vehicle+'_Idle']),
            np.arange(len(waves['SW_'+vehicle+'_Idle'])),waves['SW_'+vehicle+'_Idle'])
        drive = np.interp(pos%len(waves['SW_'+vehicle+'_Drive']),
            np.arange(len(waves['SW_'+vehicle+'_Drive'])),waves['SW_'+vehicle+'_Drive'])
        x = idle*(.4-.18*demand)+drive*(.03+.65*demand)
        x[:RATE//5] *= np.linspace(0,1,RATE//5)
        x[-RATE//2:] *= np.linspace(1,0,RATE//2)
        preview.extend([x,np.zeros(RATE)])
    save(args.output/'VehiclePreview.wav',np.concatenate(preview))
    (args.output/'VehicleAudio.json').write_text(json.dumps({'sample_rate':RATE,'channels':1,'sounds':rows},indent=2))
    print(json.dumps([{k:r[k] for k in ['name','duration_s','rms_dbfs','peak_dbfs','wrap_delta']} for r in rows]))

if __name__ == '__main__': main()

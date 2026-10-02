"""Render a quiet four-part exploration edit from the licensed NewAge export.

Usage: python render_calm_exploration.py NewAge.wav output_directory
Requires numpy. No normalization upward and no change to the marketplace asset.
"""
import hashlib
import json
import pathlib
import sys
import wave
import numpy as np

source = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
with wave.open(str(source), 'rb') as w:
    rate, channels, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    if (channels, width) != (2, 2):
        raise ValueError('Expected a stereo 16-bit PCM export')
    samples = np.frombuffer(w.readframes(w.getnframes()), '<i2').reshape(-1, 2).astype(np.float64) / 32768

def db(value):
    return float(20 * np.log10(max(float(value), 1e-12)))

def save(path, data):
    with wave.open(str(path), 'wb') as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(np.round(np.clip(data, -1, 1) * 32767).astype('<i2').tobytes())

sections, report = [], []
# Separate musical passages, reordered rather than repeating one drone.
for start in [4, 128, 66, 190]:
    x = samples[round(start * rate):round((start + 52) * rate)].copy()
    if len(x) != 52 * rate:
        raise ValueError('NewAge export is shorter than expected')
    # Pad before the zero-phase EQ to keep the ends independent.
    padded = np.pad(x, ((rate * 2, rate * 2), (0, 0)))
    freq = np.fft.rfftfreq(len(padded), 1 / rate)
    hp = 1 / np.sqrt(1 + (70 / np.maximum(freq, 1e-9)) ** 4)
    lp = 1 / np.sqrt(1 + (freq / 3200) ** 6)
    spectrum = np.fft.rfft(padded, axis=0)
    shaped = np.fft.irfft(spectrum * (hp * lp)[:, None], n=len(padded), axis=0)
    x = shaped[rate * 2:-rate * 2]
    # Slow cosine fades have zero slope at both ends, no hard edits.
    fade_in = np.sin(np.linspace(0, np.pi / 2, 7 * rate)) ** 2
    fade_out = np.cos(np.linspace(0, np.pi / 2, 9 * rate)) ** 2
    x[:len(fade_in)] *= fade_in[:, None]
    x[-len(fade_out):] *= fade_out[:, None]
    rms = np.sqrt(np.mean(x * x))
    peak = np.max(np.abs(x))
    gain = min(1.0, 10 ** (-31 / 20) / max(rms, 1e-12), 10 ** (-12 / 20) / max(peak, 1e-12))
    x *= gain
    report.append({'source_start_s':start, 'duration_s':52, 'rms_dbfs':db(np.sqrt(np.mean(x*x))), 'peak_dbfs':db(np.max(np.abs(x))), 'gain':gain})
    sections.extend([x, np.zeros((4 * rate, 2))])

suite = np.concatenate(sections)
output = out / 'SW_Exploration_CalmSuite.wav'
save(output, suite)
metadata = {'source':'/Game/MixOfAmbientMusic/Wav/NewAge_wav', 'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
    'output_sha256':hashlib.sha256(output.read_bytes()).hexdigest(), 'sample_rate':rate, 'duration_s':len(suite)/rate,
    'sections':report, 'fade_in_s':7, 'fade_out_s':9, 'pause_s':4, 'highpass_hz':70, 'lowpass_hz':3200,
    'rms_dbfs':db(np.sqrt(np.mean(suite*suite))), 'peak_dbfs':db(np.max(np.abs(suite))),
    'boundary_peak':float(max(np.max(np.abs(suite[:rate//100])), np.max(np.abs(suite[-rate//100:]))))}
(out/'CalmExploration.json').write_text(json.dumps(metadata,indent=2),encoding='utf-8')
print(json.dumps(metadata))

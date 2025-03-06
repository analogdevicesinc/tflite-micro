import os
import sys 
import numpy as np
import librosa 
SAMPLERATE = 16000

def open_wav(file, expected_sr=SAMPLERATE, verbose=False):
    data, sr = librosa.load(file, sr=expected_sr)
    if sr != expected_sr:
        if verbose:
            print(f"expected sr: {expected_sr} real: {sr} -> resampling")
        data = librosa.resample(data, orig_sr=sr, target_sr=expected_sr)
    return data


input_file=sys.argv[1]
output_file=sys.argv[2]
print(f"Converting wav file {input_file} to bin {output_file}")
input_data = open_wav(input_file)
input_data.tofile(output_file)


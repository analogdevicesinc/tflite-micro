import sys
import numpy as np
import librosa

# Check if correct number of arguments is provided before accessing sys.argv
if len(sys.argv) != 4:
    print("\nUsage: python script.py <input_wav_file> <output_bin_file> <sampling_rate>\n")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

# Convert sampling rate to integer
try:
    SAMPLERATE = int(sys.argv[3])
except ValueError:
    print("\nError: Sampling rate must be an integer.\n")
    sys.exit(1)

def open_wav(file, expected_sr, verbose=False):
    data, sr = librosa.load(file, sr=expected_sr)  # librosa.load handles resampling
    if sr != expected_sr and verbose:
        print(f"Warning: Resampled from {sr} Hz to {expected_sr} Hz")
    return data

print(f"Converting WAV file '{input_file}' to BIN file '{output_file}' at {SAMPLERATE} Hz sampling rate")

# Load and save data
input_data = open_wav(input_file, expected_sr=SAMPLERATE)
input_data.astype(np.float32).tofile(output_file)  # Ensure float32 format

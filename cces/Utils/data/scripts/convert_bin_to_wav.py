import os
import sys
import numpy as np
import soundfile as sf

# Ensure correct number of arguments before accessing sys.argv
if len(sys.argv) != 4:
    print("\nUsage: python convert_bin_to_wav.py <input_bin_file> <output_wav_file> <sampling_rate>\n")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

# Convert SAMPLERATE to integer
try:
    SAMPLERATE = int(sys.argv[3])
except ValueError:
    print("\nError: Sampling rate must be an integer.\n")
    sys.exit(1)

print(f"Converting BIN file '{input_file}' to WAV '{output_file}' at {SAMPLERATE} Hz sampling rate")

# Read binary file and convert to NumPy array
out = np.fromfile(input_file, dtype=np.float32)

# Write WAV file
sf.write(output_file, out, SAMPLERATE)
print("Conversion complete.")

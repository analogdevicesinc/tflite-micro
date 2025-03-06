import os
import sys 
import numpy as np
import soundfile as sf
SAMPLERATE = 16000


input_file=sys.argv[1]
output_file=sys.argv[2]
print(f"Converting bin file {input_file} to wav {output_file}")
out = np.fromfile(input_file, dtype='float32')
sf.write(output_file, out, SAMPLERATE)

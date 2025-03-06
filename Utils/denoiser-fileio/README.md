python library prerequisites: numpy, librosa, soundfile
can be installed with: `pip install numpy librosa soundfile`

The sampling rate of the audio sample being converted can be changed inside each of the files accordingly.

convert_wav_to_bin.py - Converts wav files to serialized binary files to be used as inputs in application projects.
Usage: `python convert_wav_to_bin.py [input_file].wav [output_file].bin`

convert_bin_to_wav.py - Converts serialized binary files back to wav files.
Usage: `python convert_bin_to_wav.py [input_file].bin [output_file].wav`

python library prerequisites: numpy, librosa, soundfile
can be installed with: `pip install numpy librosa soundfile`

The sampling rate of the audio sample being used for denoiser application is 48KHz.

1. **Sample Data**
Sample audio files are present in `sample_data` folder. 

2. **Input/Output data for File I/O application**
The sample audio file needs to be converted into binary file with 48KHz sampling rate and the binary file needs to be placed in `cces\examples\urbansound_classification\urbansound_id_fileio\src\input` as `air_conditioner.bin`.

Inorder to convert the wav files to serialized binary files to be used as inputs in application projects use the `Utils/data/scripts/convert_wav_to_bin.py`

cd Utils/data/scripts
Usage: `python convert_wav_to_bin.py [input].wav [output].bin sampling_rate`
Example: `python convert_wav_to_bin.py "..\urbansound_classification\sample_data\air_conditioner.wav" "..\..\..\examples\urbansound_classification\urbansound_id_fileio\src\input\air_conditioner.bin" 48000`

3. **Input/Output data for Realtime application** 
Play the downloaded sample wav file directly on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board. 
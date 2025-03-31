python library prerequisites: numpy, librosa, soundfile
can be installed with: `pip install numpy librosa soundfile`

The sampling rate of the audio sample being used for denoiser application is 16KHz.

1. **Sample Data**
Some sample data are already added in the sample_data folder. Incase you wish to download more, use the google speech command dataset https://huggingface.co/datasets/google/speech_commands and place then in `sample_data` folder.

2. **Input/Output data for File I/O application**
The sample audio file needs to be converted into binary file with 16KHz sampling rate and the binary file needs to be placed in `Eagle-TFLM\examples\keyword_spotter\kws_fileio\src\input` as `keyword_audio.bin`.

Inorder to convert the wav files to serialized binary files to be used as inputs in application projects use the `Utils/data/scripts/convert_wav_to_bin.py`

cd Utils/data/scripts
Usage: `python convert_wav_to_bin.py [input].wav [output].bin sampling_rate`
Example: `python convert_wav_to_bin.py "..\keyword_spotter\sample_data\go.wav" "..\..\..\examples\keyword_spotter\kws_fileio\src\input\keyword_audio.bin" 16000`

3. **Input/Output data for Realtime application** 
Play the downloaded sample wav file directly on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board.
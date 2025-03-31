python library prerequisites: numpy, librosa, soundfile
can be installed with: `pip install numpy librosa soundfile`

The sampling rate of the audio sample being used for denoiser application is 22050Hz.

1. **Sample Data**
Download the music sample 1(blues.00013.wav), sample 2(country.00008.wav), sample 3(disco.00006.wav) audio file from https://www.kaggle.com/datasets/andradaolteanu/gtzan-dataset-music-genre-classification genres_original folder, and place then in `sample_data` folder.

2. **Input/Output data for File I/O application**
The sample audio file needs to be converted into binary file with 22050Hz sampling rate and the binary file needs to be placed in `Eagle-TFLM\examples\genre_identification\genre_id_fileio\src\input` as `genre_audio.bin`.

Inorder to convert the wav files to serialized binary files to be used as inputs in application projects use the `Utils/data/scripts/convert_wav_to_bin.py`

cd Utils/data/scripts
Usage: `python convert_wav_to_bin.py [input].wav [output].bin sampling_rate`
Example: `python convert_wav_to_bin.py "..\genre_identification\sample_data\blues.00013.wav" "..\..\..\examples\genre_identification\genre_id_fileio\src\input\genre_audio.bin" 22050`

3. **Input/Output data for Realtime application** 
Play the downloaded sample wav file directly on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board. 
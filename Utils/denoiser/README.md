python library prerequisites: numpy, librosa, soundfile
can be installed with: `pip install numpy librosa soundfile`

The sampling rate of the audio sample being used for denoiser application is 16KHz.

1. **Sample Data**
Download the noisy sample 1(audioset_realrec_airconditioner_2TE3LoA2OUQ.wav), sample 2(clnsp52_amMeH4u6AO4_snr5_tl-18_fileid_19.wav), sample 3(clnsp57_bus_84241_3_snr2_tl-30_fileid_300.wav) audio file from https://github.com/breizhn/DTLN?tab=readme-ov-file#audio-samples, and place then in `sample_data` folder.

2. **Input/Output data for File I/O application**
The sample audio file needs to be converted into binary file with 16KHz sampling rate and the binary file needs to be placed in `Eagle-TFLM\examples\denoiser_fileio\src\input` as `audio_in.bin`.

Inorder to convert the wav files to serialized binary files to be used as inputs in application projects use the `Utils/denoiser/scripts/convert_wav_to_bin.py`

cd Utils/denoiser/scripts
Usage: `python convert_wav_to_bin.py [input].wav [output].bin sampling_rate`
Example: `python convert_wav_to_bin.py "..\sample_data\audioset_realrec_airconditioner_2TE3LoA2OUQ.wav" "..\..\..\examples\denoiser_fileio\src\input\audio_in.bin" 16000`

Inorder to convert the serialized binary denoised audio output file back to wav file use the `Utils/denoiser/scripts/convert_bin_to_wav.py`.
cd Utils/denoiser/scripts
Usage: `python convert_bin_to_wav.py [input].bin [output].wav sampling_rate`
Example: `python convert_bin_to_wav.py "..\..\..\examples\denoiser_fileio\src\audio_out.bin" "..\sample_data\denoised_audio.wav" 16000`

3. **Input/Output data for Realtime application** 
Play the downloaded sample wav file directly on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board. 
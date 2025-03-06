# DTLN Denoiser
This folder contains the SharcFX port of the DTLN(Dual-signal Transformation LSTM Network) denoiser application. 

Original repo: https://github.com/breizhn/DTLN

## Details of the model (from the original repo)
The DTLN model was handed in to the deep noise suppression challenge (DNS-Challenge) and the paper was presented at Interspeech 2020.
This approach combines a short-time Fourier transform (STFT) and a learned analysis and synthesis basis in a stacked-network approach with less than one million parameters. The model was trained on 500h of noisy speech provided by the challenge organizers. The network is capable of real-time processing (one frame in, one frame out) and reaches competitive results. Combining these two types of signal transformations enables the DTLN to robustly extract information from magnitude spectra and incorporate phase information from the learned feature basis. The method shows state-of-the-art performance and outperforms the DNS-Challenge baseline by 0.24 points absolute in terms of the mean opinion score (MOS).
The [DNS Challenge](https://github.com/microsoft/DNS-Challenge/) dataset was used to train the model. 

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|✅|
|int8 quantized model|✅|
|FileIO operation |✅|
|Realtime operation |✅|

##  Run application in FileIO mode
* Follow the steps mentioned in the README file in `Eagle-TFLM\Utils\automated-model-conversion-dtln` to generate the models for the application. The batch script will download, convert both the int16 and float32 models and place them in the relevant locations for the project to access it. Ignore this step if already done for the `denoiser_realtime` project.
* This example is intended for mono audio samples of 16KHz sampling rate only. 
* Build and run the **denoiser_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 
* Expected output: denoised audio signal will be stored in the `denoiser_fileio` folder under `audio_out.bin`. 
* The output bin file `audio_out.bin` can be converted back to wav format using the `convert_bin_to_wav.py` script from the Utils folder (`Eagle-TFLM\Utils\denoiser-fileio`). Please note that this needs the packages numpy, librosa and soundfile which can be installed using 'pip install librosa numpy soundfile'. 
* The default model used for running in FP32. You can switch to int16 activations/int8 weights model by enabling the macro 'DO_QUANTIZED_INFERENCE' present in 'main_functions.cc'
* By default, the project will use a sample input provided as the input to the model. To use your own sample as input:
	* Ensure the audio sample you wish to use as input to the model is sampled at **16000Hz**. 
	* Use the `convert_wav_to_bin.py` script from the Utils folder (`Utils\denoiser-fileio`) to convert your audio wav file into corresponding bin files. 
	* Copy the input bin files into the project folder - `examples\denoiser_fileio\src\input`.
	* Replace the name of the converted input bin file in the `OpenInputFile()` function in `src\frame_provider.cc`. 

##  Run application in Realtime mode
* Implemented as a separate project: Check `denoiser_realtime`
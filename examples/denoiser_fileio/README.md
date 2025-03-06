# DTLN Denoiser - Realtime
This folder contains the SharcFX port of the DTLN(Dual-signal Transformation LSTM Network) denoiser application for realtime operation with the board. 
This example utilizes the ADAU1979 ADC and the ADAU1962 DAC to operate in I2S mode for Audio talkthrough that has been leveraged to run the DTLN denoiser in realtime. The input from the ADC is downsampled and supplied to the denoiser model and the denoised output from the model is passed back to the DAC which can be heard through the Headphone connected.

Original repo: https://github.com/breizhn/DTLN

## Details of the model (from the original repo)
The DTLN model was handed in to the deep noise suppression challenge (DNS-Challenge) and the paper was presented at Interspeech 2020.
This approach combines a short-time Fourier transform (STFT) and a learned analysis and synthesis basis in a stacked-network approach with less than one million parameters. The model was trained on 500h of noisy speech provided by the challenge organizers. The network is capable of real-time processing (one frame in, one frame out) and reaches competitive results. Combining these two types of signal transformations enables the DTLN to robustly extract information from magnitude spectra and incorporate phase information from the learned feature basis. The method shows state-of-the-art performance and outperforms the DNS-Challenge baseline by 0.24 points absolute in terms of the mean opinion score (MOS).
The [DNS Challenge](https://github.com/microsoft/DNS-Challenge/) dataset was used to train the model. 

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|✅|
|FileIO operation |✅|
|Realtime operation |✅|

## Model file generation
* Model files in `common/model/model_fp` are required to build and run the application. Absence of .cc and .h files in this folder will lead to build errors.
* Follow the steps mentioned in the README file in `Eagle-TFLM\Utils\automated-model-conversion-dtln` to generate the models for the application. The batch script will download, convert the float32 models and place them in the relevant locations for the project to access it. 
* This is the first step to run the `denoiser_fileio` or the `denoiser_realtime` project. It needs to be done only once for denoiser application.

## Data Input/Output generation
* This example is intended for mono audio samples of 16KHz sampling rate only. 
* Expected input: Requires audio_in.bin in `denoiser_fileio/src/input` folder. Follow the Readme in `Eagle-TFLM\Utils\denoiser` to generate the input file for testing the application.
* Expected output: denoised audio signal will be stored in the `denoiser_fileio/src/` folder under `audio_out.bin`. 
* The output bin file `denoised_audio.bin` can be converted back to wav format using the `convert_bin_to_wav.py` script from the Utils folder (`Eagle-TFLM\Utils\denoiser\scripts`). Please follow the Readme in `Eagle-TFLM\Utils\denoiser`. 

##  Run application in FileIO mode
* Open CCES and import the **denoiser_fileio** project into your CCES workspace. 
* Build and run the **denoiser_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 

##  Run application in Realtime mode
* Implemented as a separate project: Check `denoiser_realtime`
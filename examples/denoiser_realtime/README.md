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
* Expected Input: Follow the Readme in `Eagle-TFLM\Utils\denoiser` to download and use the input file. You can use any custom noisy file of your choise for testing.

##  Run application in FileIO mode
* Implemented as a separate project: Check `denoiser_fileio`

##  Run application in Realtime mode

### Hardware Setup
* This example uses the I2S loopback example from the BSP as reference 
* Connect an audio source input to J12.
* Connect a headphone J17 to listen audio output.
* Following connections are done using SRU.
    * DAC Clock (output) ----> SPORT4A clock (input)
    * DAC Frame Sync (output) ----> SPORT4A Frame Sync (input)
    * DAC Clock (output) ----> SPORT4B clock (input)
    * DAC Frame Sync (output) ----> SPORT4B Frame Sync (input)
    * DAC Clock (output) ----> ADC clock (input)
    * DAC Frame Sync (output) ----> ADC Frame Sync (input)
    * ADC Data (output) ----> SPORT4B D0 (input)
    * SPORT 4A D0 (output) ----> DAC data (input)
* Both the ADC and DAC are configured in I2S mode. ADAU1962a DAC is configured to provide clock and frame sync to SPORT4A, SPORT4B and ADAU1979 ADC.

### Software 
* Open CCES and import the **denoiser_realtime** project into your CCES workspace. 
* Running the project will load the application onto the board, given all the connections are made correctly. 
* Once the application has been sucessfully loaded, an audio wav file can be played on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board and the input can be suppplied by the cable connected to J12. We combine both the stereo inputs and run through the denoiser. The denoised output is played back through both channels of the output device.  
* The ADC and DAC are configured at 48KHz so the module will decimate this to 16KHz before passing through the denoiser and then interpolate the output to 48KHz before playing back from the DAC. 
* The denosied output will be available on the device connected to J17 connector. 
* By default the application is set to denoised mode. You can toggle the modes between denoising and playing audio without denoising in passthrough mode, by pressing the PB1/SW3 button. The LED9 will be on when this is in denoising mode and be off in passthrough mode. 

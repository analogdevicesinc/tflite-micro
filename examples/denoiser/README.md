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
* Follow the steps mentioned in the README file in `Eagle-TFLM\Utils\automated-model-conversion\dtln` to generate the models for the application. The batch script will download, convert the float32 models and place them in the relevant locations for the project to access it. 
* This is the first step to run the `denoiser_fileio` or the `denoiser_realtime` project. It needs to be done only once for denoiser application.

## Data Input/Output generation
* This example is intended for mono audio samples of 16KHz sampling rate only. 
* Expected input: Requires noisy_audio.bin in `denoiser_fileio/src/input` folder. Follow the Readme in `Eagle-TFLM\Utils\data\denoiser` to generate the input file for testing the application.
* Expected output: denoised audio signal will be stored in the `denoiser_fileio/src/output` folder under `denoised_audio.bin`. 
* The output bin file `denoised_audio.bin` can be converted back to wav format using the `convert_bin_to_wav.py` script from the Utils folder (`Eagle-TFLM\Utils\data\scripts`). Please follow the Readme in `Eagle-TFLM\Utils\data\denoiser`. 

##  Run application in FileIO mode
* Open CCES and import the **denoiser_fileio** project into your CCES workspace. 
* Build and run the **denoiser_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 

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

### UART Setup
* Enable or disable UART_REDIRECT form the denoiser_realtime project settings. By default UART_REDIRECT is enabled in realtime application.
	* Right Click the denoiser_realtime project in CCES, select Properties
	* Select C/C++ Builds -> Settings -> Preprocessor in SharcFX C/C++ Compiler -> Add or remove UART_REDIRECT
* The output can be directed to the UART in the following way:
	* Make sure to install CP210x USB to UART drivers. https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
	* Connect the standard-A end of the USB cable to the host PC and standard-C end to the EV-SC835-SOM Board USB-to-UART connector.
	* Go to Control Panel > Device Manager > Ports (COM & LPT). Ensure that the USB serial port is detected.
	* Determine the COM port used by the USB serial port (for example, COM5).
	* Configure a serial terminal application with the following settings:
		* Bits per second: 9600
		* Data bits: 8
		* Parity: None
		* Stop bits: 1
		* Flow control: None
	* Open a serial terminal preferably 'Termite'. Configure the Termite terminal program with above settings and also configure it to append the Line feed (LF) for each character sent from the terminal.
* Keep a note to build the TFLM library with UART_REDIRECT enable to be able to use UART redirection for realtime applications. By default its enabled in TFLM library build. 

### Software 
* Open CCES and import the **denoiser_realtime** project into your CCES workspace. 
* Running the project will load the application onto the board, given all the connections are made correctly. 
* Once the application has been sucessfully loaded, an audio wav file can be played on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board and the input can be suppplied by the cable connected to J12. We combine both the stereo inputs and run through the denoiser. The denoised output is played back through both channels of the output device.  
* The ADC and DAC are configured at 48KHz so the module will decimate this to 16KHz before passing through the denoiser and then interpolate the output to 48KHz before playing back from the DAC. 
* The denosied output will be available on the device connected to J17 connector. 
* By default the application is set to denoised mode. You can toggle the modes between denoising and playing audio without denoising in passthrough mode, by pressing the PB1/SW3 button. The LED9 will be on when this is in denoising mode and be off in passthrough mode. 
* UART REDIRECTION support is added for realtime applications. By default UART REDIRECTION is enabled for realtime application with UART_REDIRECT macro in project settings. 
* When UART_REDIRECT is enabled the console output for denoiser is displayed on a serial Terminal(example, Termite), otherwise its displayed on the debugger IDE console window, example CCES console.
* Expected Input: Follow the Readme in `Eagle-TFLM\Utils\data\denoiser` to download and use the input file. You can use any custom noisy file of your choise for testing.
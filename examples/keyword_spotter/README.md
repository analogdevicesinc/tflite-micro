# Keyword Spotter
This folder contains the SharcFX port of a Keyword Spotter application. 


## Details of the model (from the original repo)
The model is one among a set of custom models put out by ARM-software repository specifically for Keyword Spotting applications. The model takes in audio data(in binary format) sampled at 16000Hz as input.

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|:x:|
|int8 quantized model|✅|
|FileIO operation |✅|
|Realtime operation |✅|

## Data Input/Output generation
* This example is intended for mono audio samples of 16KHz sampling rate. 
* Expected input: Requires keyword_audio.bin in `kws_fileio/src/input` folder. Follow the Readme in `Eagle-TFLM\Utils\data\keyword_spotter` to generate the input file for testing the application.
* Expected output: the detected keyword will be printed on the console. 

##  Run application in FileIO mode
* Open CCES and import the **kws_fileio** project into your CCES workspace. 
* Build and run the **kws_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 

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
* Enable or disable UART_REDIRECT form the kws_realtime project settings. By default UART_REDIRECT is enabled in realtime application.
	* Right Click the kws_realtime project in CCES, select Properties
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
* Open CCES and import the **kws_realtime** project into your CCES workspace. 
* Running the project will load the application onto the board, given all the connections are made correctly. 
* Once the application has been sucessfully loaded, an audio wav file can be played on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board and the input can be suppplied by the cable connected to J12. We combine both the stereo inputs and run through the keyword spotter.
* The ADC and DAC are configured at 48KHz so the module will decimate this to 16KHz before passing through the keyword spotter. 
* The detected keyword output will be printed on the Uart serial console. 
* The default model used for running has small int8 quantized model. You can switch to int8 quantized model as used by kws_fileio application by disabling 'USE_DS_CNN_SMALL_INT8_MODEL' in 'src\adi_run_kws.cpp'.
* UART REDIRECTION support is added for realtime applications. By default UART REDIRECTION is enabled for realtime application with UART_REDIRECT macro in project settings. 
* When UART_REDIRECT is enabled the output for **kws_realtime** is displayed on a serial Terminal(example, Termite), otherwise its displayed on the debugger IDE console window, example CCES console.
* The sample wav files from `Eagle-TFLM\Utils\data\keyword_spotter\sample_data` can be used for testing the realtime application.


#### Issues
Missed/Inconsistent detections might be obsereved in case of multiple keywords being uttered within an interval of 1s. This is a known issue and will be fixed. 

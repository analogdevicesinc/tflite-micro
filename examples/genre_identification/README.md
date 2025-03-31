# Genre Identification
This folder contains the SharcFX port of a Genre Identification application.
Original repo: https://github.com/cetinsamet/music-genre-classification/

## Details of the model (from the original repo)
The model is a modified version of VGG16 conv model. The model takes in audio data(in binary format) sampled at 22050Hz as input.

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|✅|
|int8 quantized model|✅|
|FileIO operation |✅|
|Realtime operation |x|

## Model file generation
* Model files in `common/model/int8_genre_ID` and `common/model/float_genre_ID` are required to build and run the application. Absence of .cc and .h files in this folder will lead to build errors.
* Follow the steps mentioned in the README file in `Eagle-TFLM\Utils\automated-model-conversion\genre_identification` to generate the models for the application. The batch script will download, convert the float32 and int8 models and place them in the relevant locations for the project to access it. 
* This is the first step to run the `genre_id_fileio` or the `genre_id_realtime` project. It needs to be done only once for genre_identification application.

## Data Input/Output generation
* This example is intended for mono audio samples of 22050Hz sampling rate. 
* Expected input: Requires genre_audio.bin in `genre_id_fileio/src/input` folder. Follow the Readme in `Eagle-TFLM\Utils\data\genre_identification` to generate the input file for testing the application.
* Expected Output: The detected genre for the input sample will be printed on the console at every ~3 seconds. 

##  Run application in FileIO mode
* Follow the steps mentioned in the README file in `Eagle-TFLM\Utils\automated-model-conversion\genre_identification\` to generate the models for the application. The batch script will download, convert both the int8 and float32 models and place them in the relevant locations for the project to access it. In our case in "genre-identification\common\model" folder.
* Open CCES and import the **genre_id_fileio** project into your CCES workspace. 
* Build and run the **genre_id_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 
* By default, the int8 model will be enabled to run. To use the float32 model instead, the **`DO_QUANTIZED_INFERENCE`** macro in `src\adi_run_genre_id.cpp` needs to be disabled.


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
* Enable or disable UART_REDIRECT form the genre_id_realtime project settings. By default UART_REDIRECT is enabled in realtime application.
	* Right Click the genre_id_realtime project in CCES, select Properties
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
* Open CCES and import the **genre_id_realtime** project into your CCES workspace. 
* Running the project will load the application onto the board, given all the connections are made correctly. 
* Once the application has been sucessfully loaded, an audio wav file can be played on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board and the input can be suppplied by the cable connected to J12. We combine both the stereo inputs and run through the genre_identification. The genre_identification input is played back through both channels of the output device connected to J17.  
* The ADC and DAC are configured at 48KHz so the module will decimate this to 24KHz before passing through the genre-identification and then interpolate the output to 48KHz before playing back from the DAC. 
* The genre_identification output will be displayed on the UART serial terminal. 
* By default the application is set to genre_identification mode. You can toggle the modes between genre-identification and playing audio in passthrough mode, by pressing the PB1/SW3 button. The LED9 will be on when this is in genre-identification mode and be off in passthrough mode. 
* The default model used for running in FP32. You can switch to int16 activations/int8 weights model by enabling the macro 'DO_QUANTIZED_INFERENCE' present in 'src/adi_run_genre_id.cpp'
* UART REDIRECTION support is added for realtime applications. By default UART REDIRECTION is enabled for realtime application with UART_REDIRECT macro in project settings. 
* When UART_REDIRECT is enabled the output for genre-identification is displayed on a serial Terminal(example, Termite), otherwise its displayed on the debugger IDE console window, example CCES console.
* Expected Input: Follow the Readme in `Eagle-TFLM\Utils\data\genre_identification` to download and use the input file. You can use any custom music file of your choise for testing.

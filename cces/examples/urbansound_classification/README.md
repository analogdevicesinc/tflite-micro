# Urbansound Classification
This folder contains the SharcFX port of a Urbansound Classification application.
Original repo: https://github.com/cetinsamet/music-genre-classification/

## Details of the model (from the original repo)
The model is a modified version of VGG16 conv model. The model takes in audio data(in binary format) sampled at 48KHz as input.

## Additional details
|Content|Supported?|
|:--------|:----------:|
|int8 quantized model|✅|
|FileIO operation |✅|
|Realtime operation |✅|

## Model file
* Model files in `common/model/int8_urbansound_ID` is required to build and run the application. Absence of .cc and .h files in this folder will lead to build errors.
* This is the first step to run the `urbansound_id_fileio` or the `urbansound_id_realtime` project. It needs to be done only once for urbansound_classification application.

## Data Input/Output generation
* This example is intended for mono audio samples of 48KHz sampling rate. 
* Expected input: Requires air_conditioner.bin in `urbansound_id_fileio/src/input/` folder. Follow the Readme in `cces\Utils\data\urbansound_classification` to generate the input file for testing the application. 
* Expected Output: The detected urbansound for the input sample will be printed on the console at every ~2.7 seconds. 

##  Run application in FileIO mode
* Open CCES and import the **urbansound_id_fileio** project into your CCES workspace. 
* Build and run the **urbansound_id_fileio** project. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 
* By default, the int8 model will be enabled to run. 
* The binary file present in the `urbansound_id_fileio/src/input` folder will give "air_conditioner" as the output for the urbansound class.

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
* Enable or disable UART_REDIRECT form the urbansound_id_realtime project settings. By default UART_REDIRECT is enabled in realtime application.
	* Right Click the urbansound_id_realtime project in CCES, select Properties
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
* Open CCES and import the **urbansound_id_realtime** project into your CCES workspace. 
* Running the project will load the application onto the board, given all the connections are made correctly. 
* Once the application has been sucessfully loaded, an audio wav file can be played on the laptop/PC connected to the ADSPSC8xx and ADSP218xx board and the input can be suppplied by the cable connected to J12. We combine both the stereo inputs and run through the urbansound_classification. The urbansound_classification input is played back through both channels of the output device connected to J17.  
* The ADC and DAC are configured at 48KHz, data is passed via ADC to the urbansound-identification model and then the output of 48KHz is played back from the DAC. 
* The urbansound_classification output will be displayed on the UART serial terminal. 
* The default model used for running in FP32. You can switch to int16 activations/int8 weights model by enabling the macro 'DO_QUANTIZED_INFERENCE' present in 'src/adi_run_urbansound_id.cpp'
* UART REDIRECTION support is added for realtime applications. By default UART REDIRECTION is enabled for realtime application with UART_REDIRECT macro in project settings. 
* When UART_REDIRECT is enabled the output for urbansound_classification is displayed on a serial Terminal(example, Termite), otherwise its displayed on the debugger IDE console window, example CCES console.
* Expected Input: Follow the Readme in `cces\Utils\data\urbansound_classification` to download and use the input file. You can use any custom music file of your choise for testing.

### Headless build and flash
To quickstart the urbansound_id example, this project allows headless building and flashing of the application.
The commands are bash commands and need to be run in such commandline tools. Tested the commands using git bash.
To build, run the following from `urbansound_id_fileio` or `urbansound_id_realtime` directory:
```
make SHARCFX_ROOT=<cces_path>
```
To flash the realtime application, the following command can be run from `urbansound_id_realtime` directory.
```
make flash SHARCFX_ROOT=<cces_path>
```
Before flashing, make sure to switch to boot mode 0 (No boot) then reset the board. 
Verify that the application is running by switching to boot mode 1 (SPI boot) then resetting the board again.

Note that this requires the `libTFLM.a` and `libadi_sharcfx_nn.a` static library to be built first. You can build the library by running the top-level Makefile.

To clean the built objects:
```
make clean
```

#### Building and flashing on CCES 3.0.2 and beyond
To build and flash with CCES > 3.0.2, `libTFLM.a` and `libadi_sharcfx_nn.a` must be built first using the same toolchain version. From the tensorflow lite micro root directory, run

```
make SHARCFX_ROOT=/c/analog/cces/3.0.2
cd urbansound_id_realtime
make flash SHARCFX_ROOT=/c/analog/cces/3.0.2
```

#### Troubleshooting

Incase of the following error:
1. 
```
bash: make: command not found
```
Use:
```
/c/analog/cces/<cces_version>/make.exe <command>
```

2. 
```
Error: cannot connect to ICE-xxxx emulator
```
To choose the right debugger ICE-1000 or ICE-2000, use.
```
make flash DEBUGGER=<1000/2000>
```
3. To choose between Release build or Debug build, use

```
make CONFIG=<Release/Debug>
```
# Micro Speech
This folder contains the SharcFX port of micro_speech - An example application provided by tflite-micro. 


## Details of the model (from the original repo)
This example shows how to run inference using TensorFlow Lite Micro (TFLM) for wake-word recognition. The Micro Speech model, a less than 20 kB model that can recognize 2 keywords, "yes" and "no", from speech data. The Micro Speech model takes the spectrogram data as input and produces category probabilities.

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|✅|
|int8 quantized model|:x:|
|FileIO operation |✅|
|Realtime operation |:x:|

##  Run application in FileIO mode
* Open CCES and pull in the **micro_speech** project into your CCES workspace. 
* Build and run the **micro_speech** project and the output can be seen on the console. 
* Audio samples for use with Micro Speech models must be 1000ms in length, 16-bit PCM samples, and single channel (mono). 
* Expected Input: Requires audio samples into C++ data structure (micro_speech_audio_data.cc and micro_speech_audio_data.h) in `src\testdata` folder.
* A tool is available to convert your custom audio samples into C++ data structures that you can then use in your own wake-word application. 
* The tool can be found in the Utils folder: `Eagle-TFLM\Utils\data\scripts\generate_cc_arrays.py`. Please follow the Readme `Eagle-TFLM\Utils\data\micro-speech\` to generate the input file for testing the application.
* Expected output : The detected class of keyword(yes/no) from sample input will be printed to console.


##  Run application in Realtime mode
* Not implemented
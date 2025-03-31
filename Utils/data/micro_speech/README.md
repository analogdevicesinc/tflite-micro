python library prerequisites: numpy, pillow
can be installed with: `pip install numpy pillow`

1. **Sample Data**
The sample data no_1000ms, yes_1000ms were downloaded from https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech/testdata and place in sample_data folder.

2. **Input/Output data for File I/O application**
The sample audio file needs to be converted into C++ data structure and placed in `Eagle-TFLM\examples\micro_speech\src\testdata` as `micro_speech_audio_data.cc` and `micro_speech_audio_data.h`.

Inorder to convert the wav file to .cc array and stores it for use in micro_speech project use the `Utils/data/scripts/generate_cc_arrays.py`

cd Utils/data/scripts
Usage: `python generate_cc_arrays.py [output dir] [input wav file to convert]`
Example: `python generate_cc_arrays.py ..\..\..\examples\micro_speech\src\testdata "..\micro_speech\sample_data\yes_1000ms.wav"`

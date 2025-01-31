#Tensorflow Lite Microcontrollers for SHARC-FX

This contains the TFLM port for SHARC-FX.

This contains code from TFLM master branch as of July 2023. https://github.com/tensorflow/tflite-micro/tree/main. This was created following the instructions from https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/docs/new_platform_support.md specifically for the codes gotten from Step 1: Build TFLM Static Library with Reference Kernels.

The library and example projects were created manually with adding compiler options to enable C++11 support in the clang compiler i.e. -std=c11 and -std=c++11 options for compiler and -lstdc++11 for linker. 
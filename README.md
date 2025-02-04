# Tensorflow Lite Microcontrollers for SHARC-FX

This contains the TFLM port for SHARC-FX.

This contains code from TFLM master branch as of July 2023. https://github.com/tensorflow/tflite-micro/tree/main. This was created following the instructions from https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/docs/new_platform_support.md specifically for the codes gotten from Step 1: Build TFLM Static Library with Reference Kernels.

The library and example projects were created manually with adding compiler options to enable C++11 support in the clang compiler i.e. -std=c11 and -std=c++11 options for compiler and -lstdc++11 for linker. 

## Prerequisites 
- CrossCore Embedded Studio (CCES) 3.0.0
- Make (> 4.3.0)

## SHARC-FX
### Build library
To build the library archive for TFLM with the optimized kernels, run
```
make
```

By default, the build script uses `/c/analog/cces` as the CCES search path for
the SHARC-FX toolchains and the default target is for the [ADSP-SC835](https://www.analog.com/en/products/adsp-sc835.html). </br> `RELEASE` is the default configuration but you may also select to `DEBUG` configuration mode.
To configure these, run
```
make CCES=</path/to/cces> TARGET=<DEVICE_NAME> CONFIG=<CONFIG_MODE>
```
The script will output a `./build/libTFLM.a` library which can be linked to other projects
using the `-lTFLM` flag. 
### Clean library
Build objects are stored in the `./build` directory. To remove these, run
```
make clean
```

## License
[Apache License 2.0](LICENSE)
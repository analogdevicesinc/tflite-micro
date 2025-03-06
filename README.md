# Tensorflow Lite for Microcontrollers

This repository contains for the TensorFlow Lite for Microcontrollers port supporting ADI microcontrollers and digital signal processors. 

## License
[Apache License 2.0](LICENSE)

## About

This repository contains code from TFLM master branch as of [July 2023](https://github.com/tensorflow/tflite-micro/tree/main). This port was created following the [new platform support instructions](https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/docs/new_platform_support.md). Currently, this contains support for devices based on the SHARC-FX core.
-	[ADSP-SC835 Datasheet and Product Info | Analog Devices](https://www.analog.com/en/products/adsp-sc835.html)

The library and example projects were created manually by adding compiler options to enable C++11 support in the clang compiler, i.e. -std=c11 and -std=c++11 options for compiler and -lstdc++11 for linker.

## Prerequisites 
- CrossCore Embedded Studio (CCES) 3.0.0
- Make (> 4.3.0)

## How Tos

### Build the library
To build the library archive for TFLM with the optimized kernels, run make on the top-level directory:
```
make
```

By default, the build script uses `/c/analog/cces` as the CCES search path for the SHARC-FX toolchains and the default target is for the [ADSP-SC835](https://www.analog.com/en/products/adsp-sc835.html). </br> `RELEASE` is the default configuration but you may also select to `DEBUG` configuration mode.

To configure these, run
```
make CCES=</path/to/cces> TARGET=<DEVICE_NAME> CONFIG=<CONFIG_MODE>
```
The script will output a `./build/libTFLM.a` library which can be linked to other projects using the `-lTFLM` flag. 

### Clean the generated object files
Build objects are stored in the `./build` directory. To remove these, run
```
make clean
```

## Examples
The [examples](https://github.com/analogdevicesinc/tflite-micro/tree/feature/dtln-denoiser-example/examples) folder includes the SharcFX port for the DTLN(Dual-signal Transformation LSTM Network) denoiser application. This example utilizes the ADAU1979 ADC and the ADAU1962a DAC to operate in I2S mode for Audio talkthrough that has been leveraged to run the DTLN denoiser in realtime. The input from the ADC is downsampled and supplied to the denoiser model and the denoised output from the model is passed back to the DAC which can be heard through the Headphone connected. 

There are two variants for the denoiser application: realtime operation and file I/O operation 

- Denoiser Real-time - [README](https://github.com/analogdevicesinc/tflite-micro/blob/feature/dtln-denoiser-example/examples/denoiser_realtime/README.md)
- Denoiser File I/O - [README](https://github.com/analogdevicesinc/tflite-micro/blob/feature/dtln-denoiser-example/examples/denoiser_fileio/README.md)

We have also included utilities for automated model conversion for the DTLN model. These are available in the [utils](https://github.com/analogdevicesinc/tflite-micro/tree/feature/dtln-denoiser-example/Utils) directory. 
- Automated model conversion for DTLN - [README](https://github.com/analogdevicesinc/tflite-micro/blob/feature/dtln-denoiser-example/Utils/automated-model-conversion-dtln/README.md)

## Getting Help
Please raise a GitHub Issue for support. 

## Addtional Documentation
- [User Guide](Documents/ADI_TFLITE_MICRO_SHARCFX_UsersGuide.pdf)
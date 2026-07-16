# TFLM for SHARC-FX
This contains the TFLM port for SHARC-FX.

This contains code from [TFLM master branch](https://github.com/tensorflow/tflite-micro/tree/main) as of July 2023[[commit](https://github.com/tensorflow/tflite-micro/commit/aa945a0fc6359a1ed8ecc2ca518fedcb35a7863b)]. This was created following the instructions from https://github.com/tensorflow/tflite-micro/blob/main/tensorflow/lite/micro/docs/new_platform_support.md specifically for the codes gotten from Step 1: Build TFLM Static Library with Reference Kernels.

The library and example projects were created manually with adding compiler options to enable C++11 support in the clang compiler i.e. -std=c11 and -std=c++11 options for compiler and -lstdc++11 for linker. 
Custom TFLM models will need to be converted with TF2.17 or below for reliable inference across python and C++. Our current version of TFLM does not support per-channel quantization in Linear layers.

### Headless build and flash
This project allows headless building.
The commands are bash commands and need to be run in such commandline tools. Tested the commands using git bash.
To build TFLM, run the following from this root folder:
```
make
```

Note that the `libadi_sharcfx_nn.a` static library to be built first before building TFLM library or the application.

To clean the built objects:
```
make clean
```

Steps to build the Optimized TFLM library is available in `adi_sharcfx_nn/README.md`.
Steps to build and flash the applications are in correcsponding application folder in `cces\examples\<application>`.

The various realtime applications supported:
- `denoiser dtln`
- `genre_id`
- `keyword_spotter`
- `urban_sound_classification`
- `denoiser dfn` ⚠️ *See licensing disclaimer below*

> **DFN Licensing Disclaimer:**
> The Deep Filtering Network (DFN) model is subject to third-party licensing restrictions and
> **cannot be used in commercial products without prior approval**.
> If you require a commercially licensed version of the DFN model, please contact the
> **ADI Eagle-NN team** to request a commercially approved model before use.

#### Building on CCES 3.0.2 and beyond
To build with CCES > 3.0.2,

```
cd adi_sharcfx_nn\Project
make SHARCFX_ROOT=/c/analog/cces/3.0.2
cd kws_realtime
make flash SHARCFX_ROOT=/c/analog/cces/3.0.2
```
Build all the library and application code with the same toolchain.

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

2. To choose between Release build or Debug build, use

```
make CONFIG=<Release/Debug>
```

### Multi-boot
Please follow `cces/Utils/flashing-tools/bootloader_sharcfx` for multi-stage booting functionality.

Please note that the models we use in the example applications are for DEMO purpose only. If a product needs to be created using the same models, commercial license needs to be taken, and the necessary due diligence needs to be handled by the party with the rightful owners.

## Datasets
CC BY 4.0 compliant datasets for Urban Sound Classification and Music Genre Identification are available upon request for evaluation and training.

APPENDIX A - THIRD PARTY LICENSES FOR OPEN-SOURCE COMPONENTS ARE DETAILED AT THE LOCATION BELOW:
 * HTTPS://DOWNLOAD.ANALOG.COM/SHARC-FX-TFLM-EDGE-AI-SDK/VERSIONS.HTML [https://download.analog.com/sharc-fx-tflm-edge-ai-sdk/versions.html]

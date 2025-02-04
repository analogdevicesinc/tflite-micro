# Build script

This script build the TFLM library for multiple toolchains and target devices.

## Usage
```
./build.sh [--device DEVICE] [--toolchain TOOLCHAIN]
```
  * `--device DEVICE` Specify the device to build for. Available devices are:
    * ADSP-SC835
    * ADSP-SC835W
  * `--toolchain TOOLCHAIN` Specify the path of the toolchain to use.

By default, `./build.sh` runs for all permutations of toolchains and targets. To add toolchains and/or targets, modify `build.sh`

```
#!/bin/bash
devices=(
        "ADSP-SC835"
        "ADSP-SC835W"
        <NEW-TARGET>)

toolchains=(
        "/opt/analog/cces/3.0.0"
        "/opt/analog/cces/3.0.2"
        <path/to/new/toolchain>)
```
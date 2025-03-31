# Pre-requisites
Ensure the following Hardware dependencies are met

1. TWI Softconfig: Due to the internal pullups in the new softconfig IC, the default external pull downs added for the softconfig IC output signals don't take effect. Hence we need to short R28 in order for TWI1 signals to be connected to softconfig IC.
2. SYS_HWRST Not connected to DSP : SYS_HWRST signal is missing from final schematics. Appears to have been removed when temporary U1 symbol was updated to final 21835 symbol. Run wire from R75 to R57 to create connection.
3. Change R36 to 20k(for XSPI_EN default to zero)(required for XSPI Boot).
4. R111 through R120 resistors(9 resistors, 8 data lines and 1 dqs_rwds) must be replaced by a short for XSPI flash tests.

**Newer boards are shipped with these changes made, but in case you are using an earlier version of the board, please ensure the HW changes highlighted are present on the board you are using, for flash-boot to work as expected.**

5. Dial on the SOM board not set to `0` before initiating Flash Programmer.

The steps are described in 2 parts:
1. Changes to be made in the target application to make the `.ldr` file boot-ready
2. Flashing the generated loader file onto the board

---

## Changes to be made in the target application to make the `.ldr` file boot-ready
1. **Ensure boot mode is set to SPI and boot format is set to Include**

    Boot mode and boot format can be set under:  
    `Project properties > C/C++ Build > Settings > Tool Settings > CrossCore SHARC-FX Loader > General`

2. **Defining the initialization file**

    To make the loader file of the target application boot-ready, it needs an initialization sequence which is defined in:  
    `Eagle-TFLM\Utils\flashing-tools\init_code_sharc_fx`

    Define the init file path under:  
    `Project properties > C/C++ Build > Settings > Tool Settings > CrossCore SHARC-FX Loader > Initialization`

    Ensure the entire path to the `init_code_sharc_fx.dxe` file (generated after running the `init_code_sharc_fx CCES` application) is added. `init_code_sharc_fx.dxe` gets generated in Release or Debug folder after building `Eagle-TFLM\Utils\flashing-tools\init_code_sharc_fx` CCES porject.

3. **Build target project with these additional settings**

    This will generate a loader file (`.ldr`) in the Release or Debug folder of the application project. This also contains the required initialization sequence.

---

## Flashing the generated loader file onto the board
1. **Bring in the loader file**
	Copy the built loader file(`.ldr`) to `Eagle-TFLM\prebuilts`.
	
    By default, the flash programmer will be configured to use the prebuilt loader file for the DTLN denoiser application.  
    This can be changed by replacing it with the path to the loader file you wish to flash onto the board inside the `Flash_Program_EHP_SOM CCES` project under `adi_sharcfx_flash_boot.c`.

2. **Build and Run the Flash Programmer**

    Build the `Flash_Program_EHP_SOM CCES` project.

    Run/Debug the built project via the **Launch Group configuration** as would be done while loading any other CCES application on the hardware. Please follow the steps documented in CrossCore®EmbeddedStudio_SHARC-FX_Getting_Started_Guide.pdf in section 7 and 7.2.

    Once flashing is complete, the console will display the following:  
    At this point, the flash has been loaded with your target application and the program can be exited.

---

## Verifying functionality
To test if it is successfully booting from flash or not, set the dial on the SOM board to "1" and press the reset button (next to `PB1`).  
Your target application should get loaded automatically and the functionality can be verified as expected.

---

## Debugging
**Nothing happening after the console prints "Erasing the blocks"**

- **Most likely cause:** Dial on the SOM board not set to `0` before initiating Flash Programmer.
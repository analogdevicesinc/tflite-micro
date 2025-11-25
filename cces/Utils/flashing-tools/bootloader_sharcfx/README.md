# MultiBoot Flashing Instructions – ADSP-SC83x SPI Flash (SPI Master)

This directory contains instructions for flashing all the application ldr files and the bootloader **.ldr** (loader) file for multi-stage booting. 
The various applications supported:
- `denoiser`
- `genre_identification`
- `urbansound_classification`
- `keyword_spotter`

These loader files can be flashed to the SPI flash memory on the **ADSP-SC835W SOM** board using **Command Line Device Programmer (cldp)**.

Please build `cces/Utils/flashing-tools/bootloader_sharcfx` application via CCES in Release mode. This will generate the dxe application file.
Post that please follow steps to convert it to ldr file and then steps to flash it to the hardware.
---

#### Headless build using CCES 3.0.2 and beyond

```
make SHARCFX_ROOT=/c/analog/cces/3.0.2
```

#### Troubleshooting build

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

## ⚙️ Flashing the Loader Files

Follow these steps to flash a prebuilt loader file to the SPI flash:

### 🧭 Preparation
1. Build the required supported application in Release mode following steps in README's in respective folders in `cces/examples`. Make sure the loader files(ldr) are available for all the applications.
2. Turn the **dial to '0'** on the SOM board (ensures booting from SPI master).
3. Connect the ICE-1000/ICE-2000 emulator to the SOM board and host PC.
4. All the commands run in the UNIX command line tools. Commands were tested using git bash.

### 🖥️ Flash Command Format

```sh
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -file <ldr_file>
```
Note:
1. Replace <ldr_file> with your actual .ldr file name.
2. Use -emu 1000 for ICE-1000 or -emu 2000 for ICE-2000.
3. Use -offset 0x<address> if flashing at a non-default offset (e.g., for multi-stage booting).
4. This code is written for ADSP-SC835 only and tested with CCES 3.0.2.

Example Usage:

... -offset 0x80000 -file ./denoiser_realtime.ldr

###️ 🛠Generating .ldr from .dxe
```sh
C:/analog/cces/3.0.2/elfloader \
  -proc ADSP-SC835 \
  -si-revision 0.0 \
  -bspi \
  -fbinary \
  -bcode 1 \
  -init "C:/analog/cces/3.0.2/Xtensa/SHARC-FX/ldr/ADSPSC835W-EV-SOM_initcode_core1.dxe" \
  -core1="<dxe_file>" \
  -verbose \
  -o <output_ldr_file>
```
Note:
1. Replace <dxe_file> with your actual input dxe file name.
2. Replace <output_ldr_file> with your actual .ldr file name.

## 🚀 Multi-Stage Booting – ADSP-SC835 SPI Flash

In a multi-stage boot configuration, a **bootloader** is flashed at the base address (0x0), and multiple applications are flashed at different offsets. The bootloader handles selection and launching of these applications.

---

### 🧭 Flashing Multi-Stage Bootloader and Applications

Use the following `cldp` commands to flash each component to its respective address. Make sure the loader files are in the directory from where these steps will be run.
Prebuilt loader files for the applications can be found in `cces/prebuilts` directory. For ease of use there is a `flash.sh` script present to do these steps in one go. Incase the example applications were changed and rebuilt, please run `copy_ldr.sh` before running the `flash.sh` which ensures the newly built loader files are used.

```sh
# 🔹 Step 1: Flash the bootloader at base address 0x00000000
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -file ./bootloader_sharcfx.ldr

# 🔹 Step 2: Flash the Denoiser app at offset 0x00080000
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -offset 0x00080000 \
  -file ./denoiser_realtime.ldr

# 🔹 Step 3: Flash the Genre ID app at offset 0x00513e00
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -offset 0x00513e00 \
  -file ./genre_id_realtime.ldr

# 🔹 Step 4: Flash the Urbansound Classification app at offset 0x009a7c00
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -offset 0x009a7c00 \
  -file ./urbansound_id_realtime.ldr


# 🔹 Step 5: Flash the Keyword Spotter (KWS) app at offset 0x00E3BA00
C:/analog/cces/3.0.2/cldp \
  -proc ADSP-SC835 \
  -emu 2000 \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/SC83x_flash.dxe \
  -offset 0x00E3BA00 \
  -file ./kws_realtime.ldr
```

### Running the Bootloader
Once flashing is complete:

1. Turn the boot mode dial to 1 (to boot from SPI Master).
2. Press the reset button on the SOM board.
3. The bootloader starts and displays the following menu over UART serial.

### 📟 UART Bootloader Menu Output

When the board boots with the multi-stage loader, the following message is displayed over UART:

Hi it's the Bootloader
*****************************************
Press Push Button 1 for Genre ID application 
Press Push Button 2 for UrbanSound application
Press Push Button 1 and 2 both for KWS application 
Incase within 2 seconds no button is pressed Denoiser application will start running 
Reboot to come to the selection menu again

*****************************************

### UART Setup
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

Note: 
In windows, git bash was used to run the commands.
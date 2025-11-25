# Prebuilts Flashing Instructions – ADSP-SC83x SPI Flash (SPI Master)

This directory contains prebuilt **.ldr** (loader) files for the following SHARC-FX-based projects:
- `denoiser`
- `genre_identification`
- `urbansound_classifcation`
- `keyword_spotter`
- `bootloader_sharcfx`

These loader files can be flashed to the SPI flash memory on the **ADSP-SC835W SOM** board using **Command Line Device Programmer (cldp)**.

For ease a `copy_ldr.sh` script is added to copy all the built ldr files from their respective project build directory to prebuilts folder and a `flash.sh` script is provided which flashes all the 4 application along with the bootloader application to flash in one go.

---

## ⚙️ Flashing the Loader Files

Follow these steps to flash a prebuilt loader file to the SPI flash:

### 🧭 Preparation
1. Turn the **dial to '0'** on the SOM board (ensures booting from SPI master).
2. Connect the ICE-1000/ICE-2000 emulator to the SOM board and host PC.

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
3. Use -offset 0x80000 if flashing at a non-default offset (e.g., for multi-stage booting).

Example Usage:

... -offset 0x80000 -file app_<ldr_file>

By default if you need to run the application as soon as you turn dial to 1 and press reset button, then dont use any offset.

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

### Running the Application
Once flashing is complete:

1. Turn the boot mode dial to 1 (to boot from SPI Master).
2. Press the reset button on the SOM board.
3. The bootloader starts and displays the following menu over UART serial.

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
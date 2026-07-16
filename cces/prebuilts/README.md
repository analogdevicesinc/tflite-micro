# Prebuilts Flashing Instructions – ADSP-SC83x SPI Flash (SPI Master)

This directory contains prebuilt **.ldr** (loader) files for the following SHARC-FX-based projects:
- `denoiser_dtln` — flash offset `0x00080000`
- `genre_identification` — flash offset `0x00513E00`
- `urbansound_classification` — flash offset `0x009A7C00`
- `keyword_spotter` — flash offset `0x00E3BA00`
- `bootloader_sharcfx` — flash offset `0x00000000` (base)

**Flash map summary:**

| Application | Flash offset | .ldr file |
|---|---|---|
| Bootloader | `0x00000000` | `bootloader_sharcfx.ldr` |
| Denoiser DTLN | `0x00080000` | `denoiser_realtime.ldr` |
| Genre Identification | `0x00513E00` | `genre_id_realtime.ldr` |
| Urban Sound Classification | `0x009A7C00` | `urbansound_id_realtime.ldr` |
| Keyword Spotter | `0x00E3BA00` | `kws_realtime.ldr` |

These loader files can be flashed to the SPI flash memory on the **ADSP-SC835W SOM** board using **Command Line Device Programmer (cldp)**.

For ease a `copy_ldr.sh` script is added to copy all the built ldr files from their respective project build directory to prebuilts folder and a `flash.sh` script is provided which flashes all the 4 applications along with the bootloader in one go.

**`flash.sh` usage:**
```sh
./flash.sh [emulator_id] [cces_version]
```
| Argument | Default | Description |
|---|---|---|
| `emulator_id` | `2000` | `1000` for ICE-1000, `2000` for ICE-2000 |
| `cces_version` | `3.0.2` | Installed CCES version, e.g. `3.0.2` or `3.0.4` |

Examples:
```sh
./flash.sh                  # ICE-2000, CCES 3.0.2  (default)
./flash.sh 1000             # ICE-1000, CCES 3.0.2
./flash.sh 2000 3.0.4       # ICE-2000, CCES 3.0.4
./flash.sh 1000 3.0.4       # ICE-1000, CCES 3.0.4
```
> **CCES 3.0.4 note:** `cldp` 3.0.4 requires a `2183x_flash.inc` file alongside the flash driver `.dxe`. This file ships with CCES and should already be present. If flashing fails, verify that `2183x_flash.inc` exists next to `2183x_flash.dxe` in your CCES installation.

---

## ⚙️ Flashing the Loader Files

Follow these steps to flash a prebuilt loader file to the SPI flash:

### 🧭 Preparation
1. Turn the **dial to '0'** on the SOM board (ensures booting from SPI master).
2. Connect the ICE-1000/ICE-2000 emulator to the SOM board and host PC.

### 🖥️ Flash Command Format

```sh
C:/analog/cces/<version>/cldp \
  -proc ADSP-SC835 \
  -emu <1000|2000> \
  -cmd prog \
  -erase affected \
  -format bin \
  -driver C:/analog/cces/<version>/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe \
  -file <ldr_file>
```
Notes:
1. Replace `<version>` with your installed CCES version (e.g. `3.0.2` or `3.0.4`).
2. Replace `<ldr_file>` with your actual `.ldr` file name.
3. Use `-emu 1000` for ICE-1000 or `-emu 2000` for ICE-2000.
4. Add `-offset <hex>` when flashing at a non-base offset (see flash map above).

Example — flashing the denoiser at its offset:
```sh
... -offset 0x00080000 -file denoiser_realtime.ldr
```
By default, if you need the application to boot as soon as you turn the dial to 1 and press reset, do not use any offset.

> **CCES 3.0.4 note:** `cldp` 3.0.4 requires a `2183x_flash.inc` file alongside the flash driver `.dxe`.
> This file ships with CCES and should already be present next to the `.dxe`.
> If flashing fails, verify that `2183x_flash.inc` exists in the same folder.

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
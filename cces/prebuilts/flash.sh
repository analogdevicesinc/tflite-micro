#!/bin/bash

# Usage: ./flash.sh [emulator_id]
# Default emulator: 1000
# Examples:
#   ./flash.sh       # Uses emulator 1000
#   ./flash.sh 1000  # Uses emulator 1000
#   ./flash.sh 2000 # Uses emulator 2000

EMU_ID="${1:-1000}"

echo "Flashing to ADSP-SC835 using emulator: $EMU_ID"
echo "================================================"

C:/analog/cces/3.0.2/cldp -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe -file ./bootloader_sharcfx.ldr
echo "================================================"
echo " Bootloader flashed."
echo "================================================"
C:/analog/cces/3.0.2/cldp -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe -offset 0x00080000 -file ./denoiser_realtime.ldr
echo "================================================"
echo " Denoiser application flashed."
echo "================================================"
C:/analog/cces/3.0.2/cldp -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe -offset 0x00513e00 -file ./genre_id_realtime.ldr
echo "================================================"
echo " Genre ID application flashed."
echo "================================================"
C:/analog/cces/3.0.2/cldp -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe -offset 0x009a7c00 -file ./urbansound_id_realtime.ldr
echo "================================================"
echo " UrbanSound ID application flashed."
echo "================================================"
C:/analog/cces/3.0.2/cldp -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver C:/analog/cces/3.0.2/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe -offset 0x00E3BA00 -file ./kws_realtime.ldr
echo "================================================"
echo " KWS application flashed."
echo "================================================"

echo ""
echo "Flashing complete!"

#!/bin/bash
#
# flash.sh — Flash all SHARC-FX applications to ADSP-SC835 SPI flash
#
# Usage: ./flash.sh [emulator_id] [cces_version]
#
# Arguments (both optional):
#   emulator_id   : ICE emulator type — 1000 (ICE-1000) or 2000 (ICE-2000)
#                   Default: 2000
#   cces_version  : Installed CCES version string, e.g. 3.0.2 or 3.0.4
#                   Default: 3.0.2
#
# Examples:
#   ./flash.sh                  # ICE-2000, CCES 3.0.2  (team default)
#   ./flash.sh 1000             # ICE-1000, CCES 3.0.2
#   ./flash.sh 2000 3.0.4       # ICE-2000, CCES 3.0.4
#   ./flash.sh 1000 3.0.4       # ICE-1000, CCES 3.0.4
#
# Notes for CCES 3.0.4 users:
#   cldp 3.0.4 requires a .inc file alongside the flash driver .dxe.
#   This file ships with CCES and should already be present next to the .dxe.
#   If flashing fails with a geometry error, verify that
#   2183x_flash.inc exists in the same folder as 2183x_flash.dxe.

EMU_ID="${1:-2000}"
CCES_VER="${2:-3.0.2}"
CCES_ROOT="C:/analog/cces/${CCES_VER}"
CLDP="${CCES_ROOT}/cldp"
DRIVER="${CCES_ROOT}/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe"

echo "Flashing to ADSP-SC835  |  Emulator: ICE-${EMU_ID}  |  CCES: ${CCES_VER}"
echo "================================================"

# Verify cldp exists
if [ ! -f "$CLDP" ] && [ ! -f "${CLDP}.exe" ]; then
    echo "ERROR: cldp not found at: $CLDP"
    echo "Check that CCES ${CCES_VER} is installed at C:/analog/cces/${CCES_VER}"
    exit 1
fi

# Verify flash driver exists
if [ ! -f "$DRIVER" ]; then
    echo "ERROR: Flash driver not found at:"
    echo "  $DRIVER"
    echo "Check your CCES installation path."
    exit 1
fi

# For CCES 3.0.4+: warn if .inc is missing (cldp requires it)
INC="${DRIVER%.dxe}.inc"
if [[ "$CCES_VER" != "3.0.2" ]] && [ ! -f "$INC" ]; then
    echo "WARNING: .inc file not found at:"
    echo "  $INC"
    echo "This file is required by cldp ${CCES_VER}. Flashing may fail."
fi

"$CLDP" -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver "$DRIVER" -file ./bootloader_sharcfx.ldr
echo "================================================"
echo " Bootloader flashed."
echo "================================================"

"$CLDP" -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver "$DRIVER" -offset 0x00080000 -file ./denoiser_realtime.ldr
echo "================================================"
echo " Denoiser application flashed."
echo "================================================"

"$CLDP" -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver "$DRIVER" -offset 0x00513e00 -file ./genre_id_realtime.ldr
echo "================================================"
echo " Genre ID application flashed."
echo "================================================"

"$CLDP" -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver "$DRIVER" -offset 0x009a7c00 -file ./urbansound_id_realtime.ldr
echo "================================================"
echo " UrbanSound ID application flashed."
echo "================================================"

"$CLDP" -proc ADSP-SC835 -emu $EMU_ID -cmd prog -erase affected -format bin -driver "$DRIVER" -offset 0x00E3BA00 -file ./kws_realtime.ldr
echo "================================================"
echo " KWS application flashed."
echo "================================================"

echo ""
echo "Flashing complete!"

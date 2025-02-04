#!/bin/bash
devices=(
	"ADSP-SC835"
	"ADSP-SC835W")

toolchains=(
	"/opt/analog/cces/3.0.0"
	"/opt/analog/cces/3.0.2")

# Parse arguments
while [[ "$#" -gt 0 ]]; do
	case $1 in
		--toolchain) selected_toolchain="$2"; shift ;;
		--device) selected_device="$2"; shift ;;
		--help)
			echo "Usage: $0 [--device DEVICE] [--toolchain TOOLCHAIN]"
			echo "  --device DEVICE       Specify the device to build for. Available devices: ${devices[*]}"
			echo "  --toolchain TOOLCHAIN Specify the path of the toolchain to use."
			exit 0
			;;
		*) echo "Unknown parameter passed: $1"; exit 1 ;;
	esac
	shift
done

# Validate selected device
if [[ -n "$selected_device" ]]; then
	if [[ ! " ${devices[@]} " =~ " ${selected_device} " ]]; then
		echo "Invalid device specified: $selected_device"
		exit 1
	fi
	devices=("$selected_device")
fi

# Validate selected toolchain
if [[ -n "$selected_toolchain" ]]; then
	toolchains=("$selected_toolchain")
fi

declare -A toolchain_versions
declare -A sw

for toolchain in "${toolchains[@]}"; do
	version=$(echo $toolchain | grep -oP '\d+\.\d+\.\d+')
	software=$(echo $toolchain | grep -oP 'cces|cfs')
	toolchain_versions[$toolchain]=$version
	sw[$toolchain]=$software
done

# Iterate for all devices and toolchains
for device in "${devices[@]}"; do
	for toolchain in "${toolchains[@]}"; do
		echo "Building for device: $device, using: $toolchain"
		
		# Add here runs for the build
		make -j4 TARGET=$device CCES=$toolchain BUILD_DIR=./build/${sw[$toolchain]}/${toolchain_versions[$toolchain]}/$device
		echo
		echo
	done
done

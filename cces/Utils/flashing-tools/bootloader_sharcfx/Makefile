include ../../../../tools/sharcfx/sharcfx.mk

# Initialize project root and build directory
PROJECT := bootloader_sharcfx
PROJECT_ROOT := .
BUILD_DIR := ./build
DEBUGGER := 1000
# Shared and common source files
SHARED := ../../../examples/shared

# Add include directories
INCLUDES += \
    -I"$(PROJECT_ROOT)/src" \
    -I"$(PROJECT_ROOT)/system" \
    -I"$(PROJECT_ROOT)/system/drivers/sport" \
    -I"$(PROJECT_ROOT)/system/drivers/twi" \
    -I"$(PROJECT_ROOT)/system/services/pdma" \
    -I"$(SHARED)/system" \
    -I"$(SHARED)"

# Add C sources - match the GUI build order and structure
C_SRCS += \
    $(wildcard $(PROJECT_ROOT)/system/*.c) \
    $(wildcard $(PROJECT_ROOT)/system/pinmux/GeneratedSources/*.c) \
    $(wildcard $(PROJECT_ROOT)/system/startup_lsp/lsp/*.c) \
    $(wildcard $(PROJECT_ROOT)/system/sru/*.c) \
    $(wildcard $(PROJECT_ROOT)/src/*.c)

# Add shared sources explicitly (since these are being built by GUI)
SHARED_SRCS += \
    $(SHARED)/system/ConfigSoftSwitches_EV_21835.c \
    $(SHARED)/system/ConfigSoftSwitches_EV_21835_SOM_UART.c \
    $(SHARED)/system/ConfigSoftSwitches_EV_SC835.c \
    $(SHARED)/system/ConfigSoftSwitches_EV_SC835_SOM_UART.c \
    $(SHARED)/system/ConfigSoftSwitches_EV_SOMCRR_EZKIT.c \
    $(SHARED)/system/SoftConfig_EV_SOMCRR_EZKIT_ADAU_Reset.c \
    $(SHARED)/system/SoftConfig_EV_SOMCRR_EZKIT_ADC_DAC.c \
    $(SHARED)/system/adi_pwr_SC8xx_config.c \
    $(SHARED)/debug.c \
    $(SHARED)/uart_pinmux.c \
    $(SHARED)/uart_redirection.c

# Combine all C sources
C_SRCS += $(SHARED_SRCS)

# Add drivers and services
SYSCFG_SRCS += \
    $(DRIVER_DIR)/Source/sport/adi_sport.c \
    $(DRIVER_DIR)/Source/twi/adi_twi.c \
    $(DRIVER_DIR)/Source/uart/adi_uart.c \
    $(SERVICE_DIR)/Source/pdma/adi_pdma.c \
    $(SERVICE_DIR)/Source/gpio/adi_gpio.c \
    $(SERVICE_DIR)/Source/pwr/adi_pwr.c \
    $(SERVICE_DIR)/Source/spu/adi_spu.c

SPI_BOOT := 1  # Always build spi bootable LDR version for bootloader

include ../../../../tools/sharcfx/project.mk
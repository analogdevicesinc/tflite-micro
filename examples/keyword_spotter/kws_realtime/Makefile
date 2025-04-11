include ../../../tools/sharcfx/sharcfx.mk

# Initialize project root and build directory
PROJECT := kws_realtime
PROJECT_ROOT := .
BUILD_DIR := ./build
DEBUGGER := 1000

# Shared and common source files
SHARED := ../../shared
COMMON := ../common

# Add include directories
INCLUDES += \
	-I"$(PROJECT_ROOT)/src" \
	-I"$(PROJECT_ROOT)/system" \
	-I"$(PROJECT_ROOT)/system/drivers/sport" \
	-I"$(PROJECT_ROOT)/system/drivers/twi" \
	-I"$(PROJECT_ROOT)/system/services/pdma" \
	-I"$(COMMON)/model" \
	-I"$(COMMON)" \
	-I"$(SHARED)/system" \
	-I"$(SHARED)"

# Add C sources
C_SRCS += \
	$(wildcard $(PROJECT_ROOT)/system/*.c) \
	$(wildcard $(PROJECT_ROOT)/system/pinmux/GeneratedSources/*.c) \
	$(wildcard $(PROJECT_ROOT)/system/startup_lsp/lsp/*.c) \
	$(wildcard $(PROJECT_ROOT)/system/sru/*.c) \
	$(wildcard $(SHARED)/system/*.c) \
	$(wildcard $(SHARED)/*.c) \
	$(wildcard $(COMMON)/*.c)

# Add C++ sources (.cc)
CC_SRCS += \
	$(wildcard $(COMMON)/model/*.cc)

# Add C++ sources (.cpp)
CPP_SRCS += \
	$(wildcard $(PROJECT_ROOT)/src/*.cpp)

# Add drivers and services
SYSCFG_SRCS += \
	$(DRIVER_DIR)/Source/sport/adi_sport.c \
	$(DRIVER_DIR)/Source/twi/adi_twi.c \
	$(DRIVER_DIR)/Source/uart/adi_uart.c \
	$(SERVICE_DIR)/Source/pdma/adi_pdma.c \
	$(SERVICE_DIR)/Source/gpio/adi_gpio.c \
	$(SERVICE_DIR)/Source/pwr/adi_pwr.c \
	$(SERVICE_DIR)/Source/spu/adi_spu.c

include ../../../tools/sharcfx/project.mk
# Set up roots/directories
SHARCFX_ROOT := C:/analog/cces/3.0.2
SHARCFX_ROOT_VER := $(shell echo $(SHARCFX_ROOT) | grep -oP '\d+\.\d+\.\d+')
TFLM_ROOT := ../../../TFLM/src
NN_ROOT := ../../../adi_sharcfx_nn
SYSCFG_RDIR := Xtensa/SHARC-FX/lib/src
SYSCFG_DIR := $(SHARCFX_ROOT)/$(SYSCFG_RDIR)
DRIVER_DIR := $(SYSCFG_DIR)/drivers
SERVICE_DIR := $(SYSCFG_DIR)/services
MAKE := make

# Set up toolchains
CX := $(SHARCFX_ROOT)/ccfx++
CC := $(SHARCFX_ROOT)/ccfx
AR := $(SHARCFX_ROOT)/Xtensa/XtensaTools/bin/xt-ar
GDB := $(SHARCFX_ROOT)/Xtensa/XtensaTools/bin/xt-gdb
RUN := $(SHARCFX_ROOT)/Xtensa/XtensaTools/bin/xt-run
ELFLDR := $(SHARCFX_ROOT)/elfloader
CLDP := $(SHARCFX_ROOT)/cldp

# Set up default variables
DEVICE := ADSP-SC835
CONFIG := RELEASE

include ../../../tools/sharcfx/sharcfx.mk

# Initialize project root and build directory
PROJECT := denoiser_fileio
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
	-I"$(COMMON)/model/model_fp" \
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
	$(wildcard $(COMMON)/model/model_fp/*.cc)

# Add C++ sources (.cpp)
CPP_SRCS += \
	$(wildcard $(PROJECT_ROOT)/src/*.cpp)

include ../../../tools/sharcfx/project.mk
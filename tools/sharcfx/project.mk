# Set up flashing variables
MINRT_LSP := $(BUILD_DIR)/flash/min-rt
FLASH_ALGO := $(SHARCFX_ROOT)/ARM/openocd/share/openocd/scripts/board/flash_algorithms/2183x_flash.dxe
LSP := $(PROJECT_ROOT)/system/startup_lsp/lsp

# Check SPI_BOOT parameter (default to 0 if not set)
SPI_BOOT ?= 0

ifeq ($(filter $(DEVICE), ADSP-SC835 ADSP-SC835W), $(DEVICE))
	PRELOADER := $(SHARCFX_ROOT)/Xtensa/SHARC-FX/ldr/ADSPSC835W-EV-SOM_initcode_core1.dxe
else
	PRELOADER := $(SHARCFX_ROOT)/Xtensa/SHARC-FX/ldr/ADSP21835W-EV-SOM_initcode_core1.dxe
endif

TMP := ./tmp
SHARED_TMP := $(PROJECT_ROOT)/tmp/shared
COMMON_TMP := $(PROJECT_ROOT)/tmp/common

# Set up project targets
DXE := $(BUILD_DIR)/$(PROJECT).dxe
DXE_LDR := $(BUILD_DIR)/$(PROJECT).ldr
FLASH_DXE := $(BUILD_DIR)/flash/$(PROJECT).dxe
LDR := $(BUILD_DIR)/flash/$(PROJECT).ldr
LIBTFLM := $(TFLM_ROOT)/cces/TFLM/Lib/$(CONFIG)/libTFLM.a
LIBNN := $(NN_ROOT)/Lib/$(CONFIG)/libadi_sharcfx_nn.a

# Set up object list
OBJ_LIST := $(BUILD_DIR)/objs.txt

# Add default headers
INCLUDES += \
	-I"$(TFLM_ROOT)" \
	-I"$(TFLM_ROOT)/third_party/gemmlowp" \
	-I"$(TFLM_ROOT)/third_party/flatbuffers/include"

# Set up target objects
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst ./,,$(patsubst $(COMMON)/%.c, $(COMMON_TMP)/%.c, $(patsubst $(SHARED)/%.c,$(SHARED_TMP)/%.c,$(C_SRCS)))))
CC_OBJS = $(patsubst %.cc,$(BUILD_DIR)/%.o,$(subst ./,,$(patsubst $(COMMON)/%.cc, $(COMMON_TMP)/%.cc, $(patsubst $(SHARED)/%.cc,$(SHARED_TMP)/%.cc,$(CC_SRCS)))))
CPP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(subst ./,,$(patsubst $(COMMON)/%.cpp, $(COMMON_TMP)/%.cpp, $(patsubst $(SHARED)/%.cpp,$(SHARED_TMP)/%.cpp,$(CPP_SRCS)))))
SYSCFG_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst $(SHARCFX_ROOT)/,,$(subst ./,,$(SYSCFG_SRCS))))
SRC_OBJS = $(C_OBJS) $(CC_OBJS) $(CPP_OBJS) $(SYSCFG_OBJS)
FLASH_OBJS = $(filter-out $(BUILD_DIR)/$(subst ./,,$(LSP))/mpu_table.o ,$(SRC_OBJS)) $(MINRT_LSP)/mpu_table.o

# Set base flags based on build configuration
ifeq ($(CONFIG), Release)
	FLAGS = -proc $(DEVICE) -si-revision 0.0 -c -O3 -LNO:simd -ffunction-sections -fdata-sections -mlongcalls -DCORE1 -DUART_REDIRECT -DNDEBUG $(INCLUDES) -MMD -MP
else
	FLAGS = -proc $(DEVICE) -si-revision 0.0 -c -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -g -DCORE1 -D_DEBUG $(INCLUDES) -MMD -MP
endif

# Set compiler flags
CC_FLAGS = $(FLAGS) -std=c++11 -stdlib=libc++
C_FLAGS = $(FLAGS)
L_FLAGS =  -Wl,--gc-sections -Wl,--defsym=__prefctl_default=0x88 -L$(dir $(LIBTFLM)) -L$(dir $(LIBNN)) -lstdc++11 -stdlib=libc++
CLDP_FLAGS = -cmd prog -erase affected -format bin -driver $(FLASH_ALGO)

# Default rule
all: build

# Rule to clean build directory
clean: 
	@rm -rf $(BUILD_DIR)
	@echo '[RM] $(BUILD_DIR)'
	@echo '[RM] $(TMP)'
	@rm -rf $(TMP)

prebuild: $(TMP) $(MINRT_LSP) check-model-files
	@echo '[MK] Building $(PROJECT) for $(DEVICE) in $(CONFIG) configuration'
	@echo 'Using SHARCFX toolchain from $(SHARCFX_ROOT)'
	@echo 'SPI_BOOT=$(SPI_BOOT)'

# Add model file check target
check-model-files:
	@echo '[CHK] Checking for required model files...'
	if [ -d "$(PROJECT_ROOT)/../common/model" ]; then \
			python "$(PROJECT_ROOT)/../../../Utils/automated-model-conversion/check_for_model_file.py" "$(PROJECT_ROOT)/../common/model"; \
	else \
			echo '[SKIP] Model directory not found, continuing build...'; \
	fi

build: prebuild
	@$(MAKE) --no-print-directory build-target
	@rm -rf $(TMP)
	@echo 'SPI_BOOT=$(SPI_BOOT)'
	@if [ $(SPI_BOOT) = 1 ]; then \
		echo '[MK] SPI_BOOT mode - building DXE and LDR files'; \
		$(ELFLDR) -proc $(DEVICE) -si-revision 0.0 -bspi -fbinary -bcode 1 -init "$(PRELOADER)" -core1="$(DXE)" -o "$(DXE_LDR)"; \
		echo '[MK] Built: $(DXE_LDR)'; \
		echo '[MK] Built: $(DXE)'; \
	else \
		echo '[MK] FILE I/O mode - skipping flash'; \
	fi

# Rule for flashing to target
flash: prebuild
	@$(MAKE) --no-print-directory flash-target
	@echo '[DP] Flashing to $(DEVICE) using $(DEBUGGER)'
	@$(CLDP) -proc $(DEVICE) -emu $(DEBUGGER) $(CLDP_FLAGS) -file $(LDR)


build-target: $(DXE)

flash-target: $(LDR)

# Rule to build C objects
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo '[CC] $<'
	@$(CC) $(C_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

# Rule to build C++ (.cc) objects
$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CX) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

# Rule to build C++ (.cpp) objects
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CX) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

# Rule to build driver objects
$(BUILD_DIR)/$(SYSCFG_RDIR)/%.o: $(SYSCFG_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo '[CC] $<'
	@$(CC) $(C_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

# Rule to compile to executable (.dxe)
$(DXE): $(SRC_OBJS) $(LIBTFLM) $(LIBNN)
	@echo '[CX] Linking target to $@'
	@echo '[CX] Invoking: CrossCore SHARC-FX C++ Linker'
	@$(CX) -proc $(DEVICE) -si-revision 0.0 -mlsp="$(LSP)" $(L_FLAGS) -o  "$(DXE)" @$(OBJ_LIST) -lTFLM -ladi_sharcfx_nn -lm
	@echo '[CX] Finished building target: $@'

$(TMP):
	@mkdir -p $(TMP)
	@echo '[MK] Fetching shared and common sources'
	@if [ -d "$(SHARED)" ]; then \
		cp -r --no-preserve=timestamps $(SHARED) $(SHARED_TMP); \
		echo '[CP] $(SHARED) -> $(SHARED_TMP)'; \
    else \
        echo '[ERROR] SHARED directory not found: $(SHARED)'; \
        mkdir -p $(SHARED_TMP); \
    fi
	@if [ -n "$(COMMON)" ] && [ -d "$(COMMON)" ]; then \
        cp -r --no-preserve=timestamps $(COMMON) $(COMMON_TMP); \
        echo '[CP] $(COMMON) -> $(COMMON_TMP)'; \
    else \
        echo '[SKIP] COMMON directory not defined or does not exist: $(COMMON)'; \
        mkdir -p $(COMMON_TMP); \
    fi

# Rule to set up LSP for flashing
$(MINRT_LSP):
	@echo '[MK] Fetching min-rt LSP for flashing'
	@mkdir -p $(dir $(MINRT_LSP))
	@cp -r --no-preserve=timestamps $(SHARCFX_ROOT)/Xtensa/SHARC-FX/lsp/$(DEVICE)/min-rt $(dir $(MINRT_LSP))
	@echo '[CP] /Xtensa/SHARC-FX/lsp/$(DEVICE)/min-rt -> $(dir $(MINRT_LSP))'

# Rule to compile to executable (.dxe) for flashing
$(FLASH_DXE): $(FLASH_OBJS)
	@mkdir -p $(dir $@)
	@echo '[CX] Linking target to $@'
	@echo '[CX] Invoking: CrossCore SHARC-FX C++ Linker'
	@$(CX) -proc $(DEVICE) -si-revision 0.0 -mlsp="$(MINRT_LSP)" $(L_FLAGS) -o  "$(FLASH_DXE)" $(FLASH_OBJS) -lTFLM -ladi_sharcfx_nn -lm
	@echo '[CX] Finished building target: $@'

# Rule to compile loader image (.ldr) for flashing
$(LDR): $(FLASH_DXE)
	@echo '[LDR] Creating loader image for SPI boot mode'
	@$(ELFLDR) -proc $(DEVICE) -si-revision 0.0 -bspi -fbinary -bcode 1 -init "$(PRELOADER)" -core1="$(FLASH_DXE)"
	@echo '[LDR] Loader image created: $@'

.PHONY: all clean run flash
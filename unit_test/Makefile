CCES = /c/analog/cces/3.0.0
TARGET = ADSP-SC835
CONFIG = DEBUG
BUILD_DIR := ./build
UNIT_TEST_DXE := unit_test.dxe
OBJ_LIST := ./build/objs.txt
TFLM_DIR := ../TFLM/src

CCFX++ := $(CCES)/ccfx++
CCFX := $(CCES)/ccfx
XTAR := $(CCES)/Xtensa/XtensaTools/bin/xt-ar
XTGDB := $(CCES)/Xtensa/XtensaTools/bin/xt-gdb
XTRUN := $(CCES)/Xtensa/XtensaTools/bin/xt-run

INCLUDES += \
	-I"./src" \
	-I"./system" \
	-I"./services/int" \
	-I"$(TFLM_DIR)/" \
	-I"$(TFLM_DIR)/third_party/gemmlowp" \
	-I"$(TFLM_DIR)/third_party/flatbuffers/include" \
	-I"../examples/shared" \
	-I"../adi_sharcfx_nn/Include"

C_SRCS += \
	$(wildcard ./system/sru/*.c) \
	$(wildcard ./system/pinmux/GeneratedSources/*.c) \
	$(wildcard ./system/startup_lsp/lsp/*.c) \
	$(wildcard ./system/*.c) \
	$(wildcard ./services/int/adi_int.c) \
	$(wildcard ./services/int/adi_sec.c) \
	$(wildcard ./src/*.c)

CC_SRCS += \
	$(wildcard ./src/*.cc)

CPP_SRCS+= \
	$(wildcard ./src/*.cpp)

LSP_SRCS := \
	./system/startup_lsp/lsp/ldscripts/elf32xtensa.x \
	./system/startup_lsp/lsp/ldscripts/elf32xtensa.xbn \
	./system/startup_lsp/lsp/ldscripts/elf32xtensa.xn \
	./system/startup_lsp/lsp/ldscripts/elf32xtensa.xr \
	./system/startup_lsp/lsp/ldscripts/elf32xtensa.xu \
	./system/startup_lsp/lsp/mpu_table.c \
	./system/startup_lsp/lsp/specs


C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst ./,,$(C_SRCS)))
CC_OBJS = $(patsubst %.cc,$(BUILD_DIR)/%.o,$(subst ./,,$(CC_SRCS)))
CPP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(subst ./,,$(CPP_SRCS)))

SRC_OBJS = $(C_OBJS) $(CC_OBJS) $(CPP_OBJS)

C_DEPS := $(C_OBJS:.o=.d)
CC_DEPS := $(CC_OBJS:.o=.d)

ifeq ($(CONFIG), RELEASE)
	FLAGS = -proc $(TARGET) -si-revision 0.0 -c -O3 -LNO:simd -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -g -DCORE1 -DNDEBUG $(INCLUDES) -MMD -MP
else
	FLAGS = -proc $(TARGET) -si-revision 0.0 -c -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -g -DCORE1 -D_DEBUG $(INCLUDES) -MMD -MP
endif

CC_FLAGS = $(FLAGS) -std=c++11 -stdlib=libc++
C_FLAGS = $(FLAGS) -std=c11
RUN_FLAGS = --console --noreset --no_zero_bss --mem_model --read_delay=20 --read_repeat=1 --write_delay=20 --write_repeat=1
DXE_FLAGS = -proc $(TARGET) -si-revision 0.0 -mlsp="./system/startup_lsp/lsp" -Wl,-Map=${BUILD_DIR}/unit_test.map -Wl,--gc-sections -Wl,--defsym=__prefctl_default=0x88 -L${BUILD_DIR}/lib -lstdc++11 -stdlib=libc++ -o  ${BUILD_DIR}/$(UNIT_TEST_DXE) @$(OBJ_LIST) -lm

all:
	@$(MAKE) --no-print-directory  build
	@$(MAKE) --no-print-directory  run

build:
	@$(MAKE) --no-print-directory  pre-build
	@$(MAKE) --no-print-directory  main-build

run:
	@$(CCFX++) $(CC_FLAGS) -MF"$(BUILD_DIR)/src/main.d" -MT"$(BUILD_DIR)/src/main.o" -o  "$(BUILD_DIR)/src/main.o" "./src/main.cc"
	@echo '[CX] Finished building: ../src/main.cc'
	@$(CCFX++) $(DXE_FLAGS)
	@echo '[CX] Finished building target: $(BUILD_DIR)/$(UNIT_TEST_DXE)'
	@echo '[XT] Running target: $(BUILD_DIR)/$(UNIT_TEST_DXE)'
	@$(XTRUN) $(RUN_FLAGS) $(BUILD_DIR)/$(UNIT_TEST_DXE)

clean:
	@echo '[RM] $(BUILD_DIR)'
	@rm -rf $(BUILD_DIR)

pre-build:
	@mkdir -p ${BUILD_DIR}
	@mkdir -p ${BUILD_DIR}/lib
	@mkdir -p ${BUILD_DIR}/services
	-@echo '[CP] Copy Library'
	@cp ../build/libTFLM.a ${BUILD_DIR}/lib/libTFLM.a
	-@echo '[CP] ../build/libTFLM.a ${BUILD_DIR}/lib/libTFLM.a'
	@cp ../adi_sharcfx_nn/Lib/Release/libadi_sharcfx_nn.a ${BUILD_DIR}/lib/libadi_sharcfx_nn.a
	-@echo '[CP] ../build//libadi_sharcfx_nn.a ${BUILD_DIR}/lib//libadi_sharcfx_nn.a'

main-build: $(BUILD_DIR)/$(UNIT_TEST_DXE)

$(BUILD_DIR)/$(UNIT_TEST_DXE):  $(USER_OBJS) $(LSP_SRCS) $(SRC_OBJS)
	@echo "\"-lTFLM\"" >> $(OBJ_LIST)
	@echo "\"-ladi_sharcfx_nn\"" >> $(OBJ_LIST)
	@echo '[CX] Building target: $@'
	@echo '[CX] Invoking: CrossCore SHARC-FX C++ Linker'
	@$(CCFX++) $(DXE_FLAGS)
	@echo '[CX] Finished building target: $@'

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo '[CC] $<'
	@$(CCFX) $(C_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CCFX++) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CCFX++) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

.PHONY: all build run clean
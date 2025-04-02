CCES := /c/analog/cces/3.0.0
CCES_VERSION := $(shell echo $(CCES) | grep -oP '\d+\.\d+\.\d+')
ROOT_DIR := ./TFLM/src

TARGET := ADSP-SC835
CONFIG := RELEASE
BUILD_DIR := ./build

LIBNN := 
LIBTFLM := $(BUILD_DIR)/libTFLM.a
OBJ_LIST := $(BUILD_DIR)/objs.txt

CCFX++ := $(CCES)/ccfx++
CCFX := $(CCES)/ccfx
XTAR := $(CCES)/Xtensa/XtensaTools/bin/xt-ar

INCLUDES += \
	-I"$(ROOT_DIR)/" \
	-I"$(ROOT_DIR)/tensorflow/lite/schema" \
	-I"$(ROOT_DIR)/tensorflow/lite/kernels/internal/optimized" \
	-I"$(ROOT_DIR)/third_party" \
	-I"$(ROOT_DIR)/third_party/kissfft" \
	-I"$(ROOT_DIR)/third_party/kissfft/tools" \
	-I"$(ROOT_DIR)/third_party/flatbuffers/include" \
	-I"$(ROOT_DIR)/third_party/gemmlowp" \
	-I"$(ROOT_DIR)/third_party/gemmlowp/internal" \
	-I"$(ROOT_DIR)/third_party/ruy/ruy/profiler" \
	-I"$(ROOT_DIR)/third_party/ruy" \
	-I"$(ROOT_DIR)/third_party/gemmlowp/fixedpoint"  \
	-I"./examples/shared" \
	-I"./adi_sharcfx_nn/Include"

C_SRCS += \
	$(wildcard $(ROOT_DIR)/third_party/kissfft/tools/*.c) \
	$(wildcard $(ROOT_DIR)/third_party/kissfft/*.c) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/experimental/microfrontend/lib/*.c) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/kernels/internal/optimized/*.c) \
	$(wildcard ./examples/shared/*.c)

CC_SRCS += \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/schema/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/tflite_bridge/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/models/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/memory_planner/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/kernels/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/arena_allocator/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/micro/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/kernels/internal/reference/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/kernels/internal/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/kernels/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/experimental/microfrontend/lib/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/core/c/*.cc) \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/core/api/*.cc)

CPP_SRCS+= \
	$(wildcard $(ROOT_DIR)/tensorflow/lite/kernels/internal/optimized/*.cpp)

C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst ./,,$(C_SRCS)))
CC_OBJS = $(patsubst %.cc,$(BUILD_DIR)/%.o,$(subst ./,,$(CC_SRCS)))
CPP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(subst ./,,$(CPP_SRCS)))

SRC_OBJS = $(C_OBJS) $(CC_OBJS) $(CPP_OBJS)

C_DEPS := $(C_OBJS:.o=.d)
CC_DEPS := $(CC_OBJS:.o=.d)

ifeq ($(CONFIG), RELEASE)
	FLAGS = -proc $(TARGET) -si-revision 0.0 -c -O3 -LNO:simd -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -DCORE1 -DUART_REDIRECT -DNDEBUG $(INCLUDES) -MMD -MP
else
	FLAGS = -proc $(TARGET) -si-revision 0.0 -c -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -g -DCORE1 -D_DEBUG $(INCLUDES) -MMD -MP
endif

CC_FLAGS = $(FLAGS) -std=c++11 -stdlib=libc++
C_FLAGS = $(FLAGS) -std=c11

all: prebuild $(LIBTFLM) postbuild

clean: 
	@echo '[RM] $(BUILD_DIR)'
	@rm -rf $(BUILD_DIR)

prebuild:
	@echo 'Building $(LIBTFLM) for $(TARGET) in $(CONFIG) configuration'
	@echo 'Using CCES Version $(CCES_VERSION)'
	@echo

postbuild:

$(LIBTFLM): $(SRC_OBJS)
	@echo '[AR] Archiving to $(LIBTFLM)'
	@$(XTAR) -r $(LIBTFLM) @$(OBJ_LIST)
	@echo
	@echo '[AR] Done. See $(LIBTFLM)'

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

.PHONY: all clean prebuild postbuild
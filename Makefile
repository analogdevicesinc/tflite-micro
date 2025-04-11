include ./tools/sharcfx/sharcfx.mk

BUILD_DIR := ./build
LIB_SRCS := ./TFLM/src
LIB := libTFLM.a

INCLUDES += \
	-I"$(LIB_SRCS)" \
	-I"$(LIB_SRCS)/tensorflow/lite/schema" \
	-I"$(LIB_SRCS)/tensorflow/lite/kernels/internal/optimized" \
	-I"$(LIB_SRCS)/third_party" \
	-I"$(LIB_SRCS)/third_party/kissfft" \
	-I"$(LIB_SRCS)/third_party/kissfft/tools" \
	-I"$(LIB_SRCS)/third_party/gemmlowp" \
	-I"$(LIB_SRCS)/third_party/gemmlowp/internal" \
	-I"$(LIB_SRCS)/third_party/flatbuffers/include" \
	-I"$(LIB_SRCS)/third_party/ruy/ruy/profiler" \
	-I"$(LIB_SRCS)/third_party/ruy" \
	-I"$(LIB_SRCS)/third_party/gemmlowp/fixedpoint"  \
	-I"./examples/shared" \
	-I"./adi_sharcfx_nn/Include"

C_SRCS += \
	$(wildcard $(LIB_SRCS)/third_party/kissfft/tools/*.c) \
	$(wildcard $(LIB_SRCS)/third_party/kissfft/*.c) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/experimental/microfrontend/lib/*.c) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/kernels/internal/optimized/*.c) \
	$(wildcard ./examples/shared/*.c)

CC_SRCS += \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/schema/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/tflite_bridge/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/models/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/memory_planner/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/kernels/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/arena_allocator/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/micro/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/kernels/internal/reference/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/kernels/internal/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/kernels/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/experimental/microfrontend/lib/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/core/c/*.cc) \
	$(wildcard $(LIB_SRCS)/tensorflow/lite/core/api/*.cc)

include ./tools/sharcfx/lib.mk
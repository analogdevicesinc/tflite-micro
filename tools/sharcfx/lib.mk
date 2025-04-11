# Set up targets
OBJ_LIST := $(BUILD_DIR)/objs.txt
LIB_BUILD := $(BUILD_DIR)/$(LIB)

# Set up target objects
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(subst ./,,$(C_SRCS)))
CC_OBJS = $(patsubst %.cc,$(BUILD_DIR)/%.o,$(subst ./,,$(CC_SRCS)))
CPP_OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(subst ./,,$(CPP_SRCS)))

SRC_OBJS = $(C_OBJS) $(CC_OBJS) $(CPP_OBJS)

# Initialize flags
ifeq ($(CONFIG), RELEASE)
	FLAGS = -proc $(DEVICE) -si-revision 0.0 -c -O3 -LNO:simd -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -DCORE1 -DUART_REDIRECT -DNDEBUG $(INCLUDES) -MMD -MP
	CFG_DIR = Release
else
	FLAGS = -proc $(DEVICE) -si-revision 0.0 -c -ffunction-sections -fdata-sections -fno-math-errno -mlongcalls -g -DCORE1 -D_DEBUG $(INCLUDES) -MMD -MP
	CFG_DIR = Debug
endif

CC_FLAGS = $(FLAGS) -std=c++11 -stdlib=libc++
C_FLAGS = $(FLAGS)

# Default rule
all: prebuild $(LIB_BUILD)

# Rule to clean build directory
clean: 
	@echo '[RM] $(BUILD_DIR)'
	@rm -rf $(BUILD_DIR)

prebuild:
	@echo 'Building $(LIB_BUILD) for $(DEVICE) in $(CONFIG) configuration'
	@echo 'Using SHARCFX toolchain from $(SHARCFX_ROOT)'

$(LIB_BUILD): $(SRC_OBJS)
	@echo '[AR] Archiving to $(LIB_BUILD)'
	@$(AR) -r $(LIB_BUILD) @$(OBJ_LIST)
	@echo
	@echo '[AR] Done. See $(LIB_BUILD)'
	@cp $(LIB_BUILD) $(LIB_SRCS)/../Lib/$(CFG_DIR)/$(LIB)
	@echo '[CP] $(LIB_BUILD) -> $(LIB_SRCS)/../Lib/$(CFG_DIR)/$(LIB)'

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo '[CC] $<'
	@$(CC) $(C_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CX) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo '[CX] $<'
	@$(CX) $(CC_FLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o  "$@" "$<"
	@echo "\"./$@\"" >> $(OBJ_LIST)

.PHONY: all clean
# Copied from https://spin.atomicobject.com/makefile-c-projects/
TARGET_EXEC ?= a.exe

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LDFLAGS := -lglfw3 -llibvulkan-1 -lm
CC := gcc
GLSLC := glslc.exe

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(BUILD_DIR)/shaders/vert.spv $(BUILD_DIR)/shaders/frag.spv | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile C source files
$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	cp C:\msys64\ucrt64\bin\glfw3.dll $(BUILD_DIR)

# Compile shader files
$(BUILD_DIR)/shaders/vert.spv: $(SRC_DIRS)/shaders/vertex_shader.vert
	glslc $< -o $@

$(BUILD_DIR)/shaders/frag.spv: $(SRC_DIRS)/shaders/fragment_shader.frag
	glslc $< -o $@ 
	


$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)/src
	$(MKDIR_P) $(BUILD_DIR)/shaders


	
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

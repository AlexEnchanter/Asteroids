# Copied from https://spin.atomicobject.com/makefile-c-projects/
TARGET_EXEC ?= a.exe

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.c )
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
LDFLAGS := -lglfw3 -llibvulkan-1 -lm 

CC:=gcc


$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	cp C:\msys64\ucrt64\bin\glfw3.dll $(BUILD_DIR)
	cp -R $(SRC_DIRS)/shaders* $(BUILD_DIR)


# c source
$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)/src
	
	
.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

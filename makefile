# Compiler and flags
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Target executable
TARGET = beehive_simulation

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
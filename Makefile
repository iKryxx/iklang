# Compiler + flags
CC := gcc
CFLAGS := -ggdb -Wall -Wextra -Wpedantic -std=c2x -O0 -MMD -MP
LDFLAGS :=

# Install paths
PREFIX  ?= /usr/local
BINDIR  := $(PREFIX)/bin
LIBDIR  := $(PREFIX)/lib/iklang

# Project structure
SRC_DIR := src
INC_DIR := inc
LIB_DIR := lib
BUILD_DIR := build
TARGET := iklc

# Include files
INCLUDES := -I$(INC_DIR) -DIKLANG_LIB_DIR=\"$(LIBDIR)\"

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Compile commands database
COMPILE_COMMANDS := $(BUILD_DIR)

# Default target
all: $(BUILD_DIR)/$(TARGET)

# Link
$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Ensure build dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Run
run:
	./$(BUILD_DIR)/$(TARGET)

# Install binary and stdlib
install: all
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR)
	install -m 755 $(BUILD_DIR)/$(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -m 644 $(LIB_DIR)/*.ikl $(DESTDIR)$(LIBDIR)/

# Uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -rf $(DESTDIR)$(LIBDIR)

# Generate build_commands.json
compdb: | $(BUILD_DIR)
	bear -- make clean all
	mv compile_commands.json $(COMPILE_COMMANDS)

# Optional: root-level link so tools find it immediately
compdb-link: compdb
	ln -sf $(COMPILE_COMMANDS) compile_commands.json

# Include dependency files
-include $(DEPS)

.PHONY: all clean install uninstall run compdb compdb-link

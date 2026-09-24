# tinyrwm - A tiny River window manager
# See LICENSE file for copyright and license details

# Project configuration
TARGET := tinyrwm
VERSION := 0.1.0

# Directories
SRC_DIR := .
BUILD_DIR := build
PROTO_DIR := protocol

# Source files
SRCS := tinyrwm.c commands.c util.c
HDRS := tinyrwm.h config.h util.h
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Protocol files
PROTO_XMLS := $(wildcard $(PROTO_DIR)/*.xml)
PROTO_HDRS := $(patsubst $(PROTO_DIR)/%.xml,$(BUILD_DIR)/%-client-protocol.h,$(PROTO_XMLS))
PROTO_SRCS := $(patsubst $(PROTO_DIR)/%.xml,$(BUILD_DIR)/%-protocol.c,$(PROTO_XMLS))
PROTO_OBJS := $(patsubst $(PROTO_DIR)/%.xml,$(BUILD_DIR)/%-protocol.o,$(PROTO_XMLS))

# Compiler and flags
CC := gcc
WAYLAND_SCANNER := wayland-scanner

# Build flags
CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -pedantic -Wall -Wextra -Wno-unused-parameter
CFLAGS += -O2 -march=native
CFLAGS += -I$(BUILD_DIR)

# Debug flags (use with 'make DEBUG=1')
ifdef DEBUG
	CFLAGS += -g -O0 -DDEBUG
	CFLAGS += -fsanitize=address,undefined
	LDFLAGS += -fsanitize=address,undefined
endif

# Dependencies via pkg-config
PKG_DEPS := xkbcommon wayland-client
CFLAGS += $(shell pkg-config --cflags $(PKG_DEPS))
LDFLAGS += $(shell pkg-config --libs $(PKG_DEPS))

# Colorized output
BOLD := \033[1m
RED := \033[31m
GREEN := \033[32m
YELLOW := \033[33m
BLUE := \033[34m
RESET := \033[0m

# Main targets
.PHONY: all
all: $(BUILD_DIR)/$(TARGET)

# Link the final binary
$(BUILD_DIR)/$(TARGET): $(OBJS) $(PROTO_OBJS) | $(BUILD_DIR)
	@echo "$(BOLD)$(GREEN)Linking$(RESET) $@"
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "$(BOLD)$(GREEN)Build complete:$(RESET) $@"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) $(PROTO_HDRS) | $(BUILD_DIR)
	@echo "$(BOLD)$(BLUE)Compiling$(RESET) $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile protocol source files
$(BUILD_DIR)/%-protocol.o: $(BUILD_DIR)/%-protocol.c $(BUILD_DIR)/%-client-protocol.h
	@echo "$(BOLD)$(BLUE)Compiling protocol$(RESET) $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Generate protocol headers
$(BUILD_DIR)/%-client-protocol.h: $(PROTO_DIR)/%.xml | $(BUILD_DIR)
	@echo "$(BOLD)$(YELLOW)Generating header$(RESET) $@"
	@$(WAYLAND_SCANNER) client-header $< $@

# Generate protocol source
$(BUILD_DIR)/%-protocol.c: $(PROTO_DIR)/%.xml | $(BUILD_DIR)
	@echo "$(BOLD)$(YELLOW)Generating source$(RESET) $@"
	@$(WAYLAND_SCANNER) private-code $< $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

.PHONY: compile_commands compile-commands
compile_commands compile-commands:
	$(MAKE) clean
	bear --output compile_commands.json -- $(MAKE) all

# Clean build artifacts
.PHONY: clean
clean:
	@echo "$(BOLD)$(RED)Cleaning$(RESET) build directory"
	@rm -rf $(BUILD_DIR)

# Install (requires root)
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin

.PHONY: install
install: $(BUILD_DIR)/$(TARGET)
	@echo "$(BOLD)$(GREEN)Installing$(RESET) to $(BINDIR)/$(TARGET)"
	@install -Dm755 $(BUILD_DIR)/$(TARGET) $(BINDIR)/$(TARGET)

.PHONY: uninstall
uninstall:
	@echo "$(BOLD)$(RED)Uninstalling$(RESET) $(BINDIR)/$(TARGET)"
	@rm -f $(BINDIR)/$(TARGET)

# Development helpers
.PHONY: run
run: $(BUILD_DIR)/$(TARGET)
	@echo "$(BOLD)$(GREEN)Running$(RESET) $(TARGET)"
	@./$(BUILD_DIR)/$(TARGET)

.PHONY: check
check:
	@echo "$(BOLD)$(BLUE)Checking dependencies...$(RESET)"
	@command -v $(CC) >/dev/null 2>&1 || { echo "$(RED)Error: $(CC) not found$(RESET)"; exit 1; }
	@command -v $(WAYLAND_SCANNER) >/dev/null 2>&1 || { echo "$(RED)Error: wayland-scanner not found$(RESET)"; exit 1; }
	@pkg-config --exists $(PKG_DEPS) || { echo "$(RED)Error: Missing pkg-config dependencies$(RESET)"; exit 1; }
	@echo "$(GREEN)All dependencies found!$(RESET)"

.PHONY: help
help:
	@echo "$(BOLD)tinyrwm Makefile$(RESET)"
	@echo ""
	@echo "Targets:"
	@echo "  $(BOLD)all$(RESET)       - Build tinyrwm (default)"
	@echo "  $(BOLD)clean$(RESET)     - Remove build artifacts"
	@echo "  $(BOLD)install$(RESET)   - Install to $(BINDIR) (requires root)"
	@echo "  $(BOLD)uninstall$(RESET) - Remove from $(BINDIR) (requires root)"
	@echo "  $(BOLD)run$(RESET)       - Build and run tinyrwm"
	@echo "  $(BOLD)check$(RESET)     - Check for required dependencies"
	@echo "  $(BOLD)help$(RESET)      - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  $(BOLD)DEBUG=1$(RESET)   - Build with debug symbols and sanitizers"
	@echo "  $(BOLD)PREFIX$(RESET)    - Installation prefix (default: /usr/local)"
	@echo ""
	@echo "Example:"
	@echo "  make DEBUG=1 run"

# Mark build outputs as precious to avoid unnecessary rebuilds
.PRECIOUS: $(BUILD_DIR)/%-protocol.c $(BUILD_DIR)/%-client-protocol.h

# Dependencies tracking (auto-generated)
-include $(OBJS:.o=.d)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< -MF $@

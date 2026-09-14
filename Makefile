# ============================================================
# TFTP Server/Client Project
# Makefile
#
# Project: Embedded-Linux-Project-
# Protocol: RFC 1350 - TFTP Protocol Revision 2
# Compiler: GCC
# Standard: C11
# ============================================================


# ------------------------------------------------------------
# Compiler
# ------------------------------------------------------------

CC = gcc


# ------------------------------------------------------------
# Project name
# ------------------------------------------------------------

TARGET = tftp


# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

SRC_DIR   = src/tftp
BUILD_DIR = build


# ------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------

CFLAGS = -Wall -Wextra -std=c11


# ------------------------------------------------------------
# Preprocessor / Include paths
# ------------------------------------------------------------
CPPFLAGS = \
        -Isrc/tftp \
        -Isrc/tftp/app \
        -Isrc/tftp/client \
        -Isrc/tftp/error \
        -Isrc/tftp/file \
        -Isrc/tftp/packet \
        -Isrc/tftp/reactor \
        -Isrc/tftp/server \
        -Isrc/tftp/timer \
        -Isrc/tftp/transfer \
        -Isrc/tftp/transport


# Linker flags
# ------------------------------------------------------------

LDFLAGS =


# ------------------------------------------------------------
# Libraries
# ------------------------------------------------------------

LDLIBS =


# ------------------------------------------------------------
# Source files
# ------------------------------------------------------------

SOURCES = \
        $(SRC_DIR)/app/main.c \
        $(SRC_DIR)/app/tftp_app.c \
        $(SRC_DIR)/client/tftp_client.c \
        $(SRC_DIR)/error/tftp_error.c \
        $(SRC_DIR)/file/tftp_file.c \
        $(SRC_DIR)/packet/tftp_packet.c \
        $(SRC_DIR)/reactor/reactor.c \
        $(SRC_DIR)/server/tftp_server.c \
        $(SRC_DIR)/timer/tftp_timer.c \
        $(SRC_DIR)/transfer/tftp_transfer.c \
        $(SRC_DIR)/transport/udp_transport.c


# ------------------------------------------------------------
# Object files
#
# Example:
#
# src/tftp/packet/tftp_packet.c
#
# becomes:
#
# build/tftp/packet/tftp_packet.o
# ------------------------------------------------------------

OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/tftp/%.o,$(SOURCES))


# ------------------------------------------------------------
# Dependency files
#
# GCC generates these automatically.
#
# Example:
#
# tftp_packet.o
#       |
#       +-- tftp_packet.d
#
# ------------------------------------------------------------

DEPS = $(OBJECTS:.o=.d)


# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

.PHONY: all

all: $(TARGET)


# ------------------------------------------------------------
# Final executable
# ------------------------------------------------------------

$(TARGET): $(OBJECTS)
	@echo "=============================================="
	@echo " Linking TFTP application"
	@echo "=============================================="
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@
	@echo ""
	@echo "Build successful: ./$(TARGET)"
	@echo ""


# ------------------------------------------------------------
# Compile C source files into object files
#
# $@ = target
# $< = first dependency
# ------------------------------------------------------------

$(BUILD_DIR)/tftp/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@


# ------------------------------------------------------------
# Include automatically generated dependency files
# ------------------------------------------------------------

-include $(DEPS)


# ------------------------------------------------------------
# Debug build
# ------------------------------------------------------------

.PHONY: debug

debug: CFLAGS += -g -O0

debug: clean
	$(MAKE) all


# ------------------------------------------------------------
# Release build
# ------------------------------------------------------------

.PHONY: release

release: CFLAGS += -O2

release: clean
	$(MAKE) all


# ------------------------------------------------------------
# Clean build files
# ------------------------------------------------------------

.PHONY: clean

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)/tftp
	rm -f $(TARGET)
	@echo "Clean complete."


# ------------------------------------------------------------
# Rebuild from scratch
# ------------------------------------------------------------

.PHONY: rebuild

rebuild: clean all


# ------------------------------------------------------------
# Show project configuration
# ------------------------------------------------------------

.PHONY: info

info:
	@echo "=============================================="
	@echo " TFTP Project Build Configuration"
	@echo "=============================================="
	@echo "Compiler    : $(CC)"
	@echo "Target      : $(TARGET)"
	@echo "Source Dir  : $(SRC_DIR)"
	@echo "Build Dir   : $(BUILD_DIR)"
	@echo "CFLAGS      : $(CFLAGS)"
	@echo "CPPFLAGS    : $(CPPFLAGS)"
	@echo "Sources     :"
	@$(foreach src,$(SOURCES),echo "  $(src)";)
	@echo "=============================================="


# ============================================================
# End of Makefile
# ============================================================


# === Configurable variables ===
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
OUTPUT := transcodine

# === Internal variables ===
SRC := $(shell find $(SRC_DIR) -name '*.c')
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
CFLAGS := -Wall -Wextra -Werror -ansi -I$(INC_DIR) -Wno-unused-result -g
LDFLAGS := -lm -I$(INC_DIR)

# === Default target ===
.PHONY: all compile run clean format

all: clean compile

# === Compile all source files ===
compile: $(OUTPUT)

# Link object files into final executable
$(OUTPUT): $(OBJ)
	@echo "Linking to create $(BUILD_DIR)/$(OUTPUT)..."
	@cc $(OBJ) -o $(BUILD_DIR)/$(OUTPUT) $(LDFLAGS)

# Compile each .c file in src/ to a .o file in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@cc $(CFLAGS) -c $< -o $@

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# === Run the built program ===
run: $(OUTPUT)
	@echo "Running $(OUTPUT)..."
	@./$(BUILD_DIR)/$(OUTPUT)

# === Clean all build artifacts ===
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(OUTPUT)

# === Formats all files in include dir and src dir ===
format:
	@clang-format -i $(SRC_DIR)/**/*.c -i $(INC_DIR)/**/*.h
# === Configurable variables ===
SRC_DIR := src
BUILD_DIR := build
OUTPUT := transcodine

# === Internal variables ===
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
CFLAGS := -Wall -Wextra -Werror -ansi
LDFLAGS := -lm

# === Default target ===
.PHONY: all compile run clean

all: clean compile

# === Compile all source files ===
compile: $(OUTPUT)

# Link object files into final executable
$(OUTPUT): $(OBJ)
	@echo "Linking to create $(BUILD_DIR)/$(OUTPUT)..."
	@cc $(OBJ) -o $(BUILD_DIR)/$(OUTPUT) $(LDFLAGS)

# Compile each .c file in src/ to a .o file in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	@cc $(CFLAGS) -c $< -o $@

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# === Run the built program ===
run: $(OUTPUT)
	@echo "Running $(OUTPUT)..."
	@./$(OUTPUT)

# === Clean all build artifacts ===
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(OUTPUT)

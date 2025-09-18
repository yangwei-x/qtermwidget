# Top-level convenience Makefile
# Usage: make            # configure + build qTerm example (if built)
#        make all        # same as default
#        make clean      # remove build directory

BUILD_DIR := build
CMAKE_ARGS := -DBUILD_EXAMPLE=ON -DQTERMWIDGET_USE_UTEMPTER=ON -DQTERMWIDGET_ENABLE_SSH=ON

.PHONY: all configure build qTerm clean

all: configure build

configure:
	@echo "Configuring with CMake..."
	cmake -B $(BUILD_DIR) -S . $(CMAKE_ARGS)

build:
	@echo "Building qTerm (and examples)..."
	cmake --build $(BUILD_DIR) -- -j$(shell nproc)

qTerm:
	@echo "Building qTerm target only..."
	cmake --build $(BUILD_DIR) --target qTerm -- -j$(shell nproc)

clean:
	@echo "Removing build directory..."
	rm -rf $(BUILD_DIR)

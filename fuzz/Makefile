BUILD_DIR = build

.PHONY : all
all: $(BUILD_DIR)
	cd $(BUILD_DIR); \
	cmake -DCMAKE_BUILD_TYPE=Debug ..

# Clean out the build directory
.PHONY : clean
clean: 
	rm -rf $(BUILD_DIR)/*

# Create build directory if it doesn't exist
$(BUILD_DIR): 
	-mkdir $(BUILD_DIR) 2>/dev/null
	-cd $(BUILD_DIR); rm -rf *

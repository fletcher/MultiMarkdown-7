BUILD_DIR = build
WIN_BUILD_DIR = build-windows
DEBUG_DIR = build-debug
TEST_DIR = build-test
XCODE_BUILD_DIR = build-xcode
XCODE_TEST_DIR = build-xcode-test
DOC_DIR = build-documentation


# The release target will perform additional optimization
.PHONY : release
release: $(BUILD_DIR)
	cd $(BUILD_DIR); \
	cmake -DCMAKE_BUILD_TYPE=Release ..


# The debug target includes additional testing functionality
.PHONY : debug
debug: $(DEBUG_DIR)
	cd $(DEBUG_DIR); \
	cmake -DCMAKE_BUILD_TYPE=Debug ..


# Create xcode project
# You can then build within XCode, or using the commands:
#	xcodebuild -configuration Debug
#	xcodebuild -configuration Release
.PHONY : xcode
xcode: $(XCODE_BUILD_DIR)
	cd $(XCODE_BUILD_DIR); \
	cmake -G Xcode ..


# Xcode archive
#	`make xcode` and then make any manual modifications
#	then `make archive`
.PHONE : archive
archive:
	cd $(XCODE_BUILD_DIR); \
	sh archive.sh


# Xcode debug variant
.PHONY : xcode-test
xcode-test: $(XCODE_TEST_DIR)
	cd $(XCODE_TEST_DIR); \
	cmake -G Xcode -DTEST=1 ..


# test target enables CuTest unit testing
.PHONY : test
test: $(TEST_DIR)
	cd $(TEST_DIR); \
	cmake -DTEST=1 -DCMAKE_BUILD_TYPE=Debug ..


# Cross-compile for Windows 64 bit
.PHONY : windows-64
windows-64: $(WIN_BUILD_DIR)
	cd $(WIN_BUILD_DIR); \
	cmake -DCMAKE_TOOLCHAIN_FILE=../tools/Toolchain-MinGW-w64-64.cmake -DCMAKE_BUILD_TYPE=Release ..


# Generate enum mapping
.PHONY : map
map: 
	cd $(BUILD_DIR); \
	../tools/enumsToPerl.pl ../src/libMultiMarkdown7.h enumMap.txt;


# Use astyle to format source code
.PHONY : astyle
astyle:
	astyle --options=.astylerc -q --recursive "src/*.h" "src/*.c" "dev/*.c" "fuzz/*.c" --exclude=src/char.c


# Build documentation using doxygen
.PHONY : documentation
documentation: $(DOC_DIR)
	cd $(DOC_DIR); \
	cmake -DDOCUMENTATION=1 ..; \
	cd ..; \
	cp override_tables.sty $(DOC_DIR); \
	doxygen $(DOC_DIR)/doxygen.conf;


# Generate a list of changes since last commit to 'master' branch
.PHONY : CHANGELOG
CHANGELOG:
	git log master..develop --format="*    %s" | sort | uniq > CHANGELOG-UNRELEASED


# Create build directory if it doesn't exist
$(BUILD_DIR): CHANGELOG
	-mkdir $(BUILD_DIR) 2>/dev/null
	-cd $(BUILD_DIR); rm -rf *


# Create debug directory if it doesn't exist
$(DEBUG_DIR): CHANGELOG
	-mkdir $(DEBUG_DIR) 2>/dev/null
	-cd $(DEBUG_DIR); rm -rf *


# Create test directory if it doesn't exist
$(TEST_DIR): CHANGELOG
	-mkdir $(TEST_DIR) 2>/dev/null
	-cd $(TEST_DIR); rm -rf *


# Build xcode directories if they don't exist
$(XCODE_BUILD_DIR): CHANGELOG
	-mkdir $(XCODE_BUILD_DIR) 2>/dev/null
	-cd $(XCODE_BUILD_DIR); rm -rf *

$(XCODE_TEST_DIR): CHANGELOG
	-mkdir $(XCODE_TEST_DIR) 2>/dev/null
	-cd $(XCODE_TEST_DIR); rm -rf *

# Create build directory if it doesn't exist
$(WIN_BUILD_DIR): CHANGELOG
	-mkdir $(WIN_BUILD_DIR) 2>/dev/null
	-cd $(WIN_BUILD_DIR); rm -rf *

# Create documentation directory if it doesn't exist
$(DOC_DIR):
	-mkdir $(DOC_DIR) 2>/dev/null
	-cd $(DOC_DIR); rm -rf *


# Clean out the build directory
.PHONY : clean
clean:
	rm -rf $(BUILD_DIR)/*;


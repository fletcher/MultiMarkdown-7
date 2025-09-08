# CMake configuration for libFuzzer testing
#
#	https://llvm.org/docs/LibFuzzer.html


cmake_minimum_required (VERSION 3.10)


# Use clang compiler

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang)

project(fuzz_mmd-7)


# Enable fuzzer for static libraries as well
# This also includes:
#	leak detection
#	address errors (e.g. reading past end of array, etc.)
#	undefined behavior detection
#
# Use compiler optimization so that fuzz behavior better matches production behavior
# (e.g. tail recursion optimization to prevent stack overflow in some circumstances)

# set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_FLAGS "-pg ${LIBFUZZER_FLAGS_BASE} -fsanitize=address,signed-integer-overflow,undefined,fuzzer")
# set(CMAKE_CXX_FLAGS "-pg -${LIBFUZZER_FLAGS_BASE} -fsanitize=address,signed-integer-overflow,undefined,fuzzer")


# Force Debug build type to speed build time
set(CMAKE_BUILD_TYPE Debug)


project (Fuzzer)


include_directories(../src)
include_directories(../src/export)
include_directories(..)

add_subdirectory(${PROJECT_SOURCE_DIR}/.. build-mmd)



# libMultiMarkdown Fuzzer

add_executable(fuzz_mmd-7
	fuzz_mmd-7.c
)

target_link_libraries(fuzz_mmd-7
	libMultiMarkdown7
)


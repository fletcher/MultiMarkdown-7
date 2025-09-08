# Cross-compile for Windows from Linux

set (CMAKE_CROSSCOMPILING "YES")

set (CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

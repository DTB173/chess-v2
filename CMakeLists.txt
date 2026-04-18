cmake_minimum_required(VERSION 3.28)

project(chess_v2
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES  OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /O2 /Oi /Ot /GL /arch:AVX2 /DNDEBUG /Zi")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /LTCG /OPT:REF /OPT:ICF /DEBUG")
else()
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -march=native -flto -DNDEBUG -fno-stack-protector -funroll-loops")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -O3 -flto")
endif()

add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src)
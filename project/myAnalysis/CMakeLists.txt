cmake_minimum_required(VERSION 3.10)

# Project name and language
project(myAnalysis LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM package
find_package(LLVM 14 REQUIRED CONFIG)
find_package(ZLIB REQUIRED)

# Add include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(include)

add_definitions(${LLVM_DEFINITIONS})

# Add source files
add_executable(myAnalysis src/main.cpp src/analysis.cpp)

# Link with LLVM libraries
llvm_map_components_to_libnames(LLVM_LIBS analysis core irreader support)
target_link_libraries(myAnalysis ${LLVM_LIBS})

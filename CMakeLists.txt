cmake_minimum_required(VERSION 3.15)
project(MinecraftInjectable)

# Add JSON library
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(${json_SOURCE_DIR}/include)

# Find Java development headers
find_package(JNI)
if (JNI_FOUND)
    message(STATUS "Found JNI: ${JNI_INCLUDE_DIRS}")
    include_directories(${JNI_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Could not find JNI. Please set JAVA_HOME to your JDK root directory.")
endif()

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/injector.cpp)

add_library(MinecraftInjectable SHARED ${SOURCES})
target_link_libraries(MinecraftInjectable ${SDL2_LIBRARIES})

add_executable(injector src/injector.cpp src/process_manager.cpp src/logger.cpp)
target_link_libraries(injector ${SDL2_LIBRARIES})

# Standalone autoclicker test
add_executable(standalone_autoclicker_test standalone_autoclicker_test.cpp)
target_link_libraries(standalone_autoclicker_test ${SDL2_LIBRARIES})
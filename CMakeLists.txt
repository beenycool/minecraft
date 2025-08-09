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

# Linux: link against X11 and Xtst for XTest synthetic input
if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if (X11_FOUND)
        include_directories(${X11_INCLUDE_DIR})
        set(EXTRA_X_LIBS ${X11_LIBRARIES})
        # Xtst might not be included in X11_LIBRARIES
        find_library(XTST_LIB Xext XTest Xtst PATH_SUFFIXES xorg)
        if (XTST_LIB)
            list(APPEND EXTRA_X_LIBS ${XTST_LIB})
        else()
            # Common fallback names
            find_library(LIB_XTST NAMES Xtst XTest)
            if (LIB_XTST)
                list(APPEND EXTRA_X_LIBS ${LIB_XTST})
            endif()
        endif()
    endif()
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/injector.cpp)

add_library(MinecraftInjectable SHARED ${SOURCES})
target_link_libraries(MinecraftInjectable ${SDL2_LIBRARIES} ${EXTRA_X_LIBS})

add_executable(injector src/injector.cpp src/process_manager.cpp src/logger.cpp)
target_link_libraries(injector ${SDL2_LIBRARIES} ${EXTRA_X_LIBS})

# Standalone autoclicker test
add_executable(standalone_autoclicker_test standalone_autoclicker_test.cpp)
target_link_libraries(standalone_autoclicker_test ${SDL2_LIBRARIES})
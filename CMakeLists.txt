cmake_minimum_required(VERSION 3.10)
project(MinecraftInjectable)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB SOURCES "src/*.cpp")

add_library(MinecraftInjectable SHARED ${SOURCES})
target_link_libraries(MinecraftInjectable ${SDL2_LIBRARIES})

add_executable(injector src/injector.cpp)
target_link_libraries(injector MinecraftInjectable ${SDL2_LIBRARIES})
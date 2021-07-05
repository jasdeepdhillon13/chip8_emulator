cmake_minimum_required(VERSION 3.10.2)
project(chip8)

set(CMAKE_CXX_STANDARD 11)

add_executable(
	chip8
	Chip8.cpp
	main.cpp
	Platform.cpp)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(chip8 ${SDL2_LIBRARIES})

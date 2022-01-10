cmake_minimum_required(VERSION 3.10)
project(convert-senknow-roms
	VERSION 1.0.0
	LANGUAGES C
)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_executable("${PROJECT_NAME}" "${PROJECT_NAME}.c")

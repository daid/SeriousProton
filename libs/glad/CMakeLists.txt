cmake_minimum_required(VERSION 3.7)
project(glad)

# GlAD link used: See glad/glad.h

add_library(
	glad STATIC
		glad/glad.c
		glad/glad.h
		glad/khrplatform.h
)

target_include_directories(glad
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad>)

#--------------------------------Installation----------------------------------
install(
	TARGETS glad
	EXPORT glad
	DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(
	EXPORT glad
	DESTINATION share/glad
)

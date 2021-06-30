cmake_minimum_required(VERSION 3.7)
project(glew LANGUAGES C)

#---------------------------------File lists-----------------------------------
set(source_files
    glew.c
)

#----------------------------------Compiling-----------------------------------
add_library(glew STATIC ${source_files})

target_include_directories(glew
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

#--------------------------------Installation----------------------------------
install(
    TARGETS glew
    EXPORT glew
    DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(
    EXPORT glew
    DESTINATION share/glew
)

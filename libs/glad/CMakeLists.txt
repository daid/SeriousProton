cmake_minimum_required(VERSION 3.7)
project(glad)

# GlAD link used:
# https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D2.1&api=gles1%3Dnone&api=gles2%3D2.0&api=glsc2%3Dnone&profile=core&extensions=GL_EXT_texture_compression_s3tc&extensions=GL_KHR_debug&extensions=GL_KHR_texture_compression_astc_ldr&localfiles=on 

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

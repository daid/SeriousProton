cmake_minimum_required(VERSION 3.2.3) #Only tested with 3.2.3 and 3.5.1 so far.
project(glew)

#---------------------------------File lists-----------------------------------
set(source_files_relative
	glew.c
)
set(source_files "")
foreach(source_file_relative ${source_files_relative})
	list(APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/${source_file_relative})
endforeach()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/.. #Because the header files are requested starting from "GL/..."
)

#----------------------------------Compiling-----------------------------------
add_library(glew STATIC ${source_files})

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

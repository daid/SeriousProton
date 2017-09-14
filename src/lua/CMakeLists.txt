cmake_minimum_required(VERSION 3.5.1) #Only tested with 3.5.1 so far.
project(Box2D)

#---------------------------------File lists-----------------------------------
set(source_files_relative
	lapi.c
	lauxlib.c
	lbaselib.c
	lbitlib.c
	lcode.c
	lcorolib.c
	lctype.c
	ldblib.c
	ldebug.c
	ldo.c
	ldump.c
	lfunc.c
	lgc.c
	llex.c
	lmathlib.c
	lmem.c
	loadlib.c
	lobject.c
	lopcodes.c
	lparser.c
	lstate.c
	lstring.c
	lstrlib.c
	ltable.c
	ltablib.c
	ltm.c
	lundump.c
	lvm.c
	lzio.c
)
set(source_files "")
foreach(source_file_relative ${source_files_relative})
	list(APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/${source_file_relative})
endforeach()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/.. #Because the header files are requested starting from "lua/..."
)

#----------------------------------Compiling-----------------------------------
add_library(lua STATIC ${source_files})

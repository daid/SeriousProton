cmake_minimum_required(VERSION 3.7)
project(lua LANGUAGES C)

#---------------------------------File lists-----------------------------------
set(source_files
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

#----------------------------------Compiling-----------------------------------
add_library(lua STATIC ${source_files})

target_include_directories(lua
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

#--------------------------------Installation----------------------------------
install(
	TARGETS lua
	EXPORT lua
	DESTINATION lib
)
install(
	DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DESTINATION include
	FILES_MATCHING PATTERN "*.h"
)
install(
	EXPORT lua
	DESTINATION share/lua
)

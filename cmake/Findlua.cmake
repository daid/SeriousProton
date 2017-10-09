#Finds an installation of Lua on your system.
#
#Usage:
# find_package(Lua)
#
#Output:
# The following variables will be defined by this script:
# - LUA_FOUND:
#       Set to TRUE when an installation of Lua was found, or to FALSE if it
#       was not found. If no installation was found, the rest of the output
#       variables will be empty.
# - LUA_LIB:
#       Path to the Lua library.
# - LUA_INCLUDE_DIR:
#		Directory containing the header files for Lua.


find_library(LUA_LIB
	NAMES lua liblua
)

find_path(LUA_INCLUDE_DIR
	NAMES lua.h
	PATH_SUFFIXES lua
)

string(FIND ${LUA_LIB} "-NOTFOUND" LIBRARY_FOUND)
string(FIND ${LUA_INCLUDE_DIR} "-NOTFOUND" HEADERS_FOUND)
if(${LIBRARY_FOUND} LESS 0 AND ${HEADERS_FOUND} LESS 0)
	set(LUA_FOUND TRUE)
	message(STATUS "Found lua!")
else()
	set(LUA_FOUND FALSE)
	message(WARNING "Could NOT find lua!")
endif()

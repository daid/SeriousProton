#Finds an installation of GLEW on your system, the OpenGL Extensions Wrangler.
#
#Usage:
# find_package(glew)
#
#Output:
# The following variables will be defined by this script:
# - GLEW_FOUND:
#       Set to TRUE when an installation of GLEW was found, or to FALSE if it
#       was not found. If no installation was found, the rest of the output
#       variables will be empty.
# - GLEW_LIB:
#       Path to the GLEW library.
# - GLEW_INCLUDE_DIR:
#		Directory containing the header files for GLEW.


find_library(GLEW_LIB
	NAMES glew liblua
)

find_path(GLEW_INCLUDE_DIR
	NAMES glew.h
	PATH_SUFFIXES GL
)

string(FIND ${GLEW_LIB} "-NOTFOUND" LIBRARY_FOUND)
string(FIND ${GLEW_INCLUDE_DIR} "-NOTFOUND" HEADERS_FOUND)
if(${LIBRARY_FOUND} LESS 0 AND ${HEADERS_FOUND} LESS 0)
	set(GLEW_FOUND TRUE)
	message(STATUS "Found GLEW!")
else()
	set(GLEW_FOUND FALSE)
	message(WARNING "Could NOT find GLEW!")
endif()

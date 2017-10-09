#Finds an installation of Box2D on your system.
#
#Usage:
# find_package(box2d)
#
#Output:
# The following variables will be defined by this script:
# - BOX2D_FOUND:
#       Set to TRUE when an installation of Box2D was found, or to FALSE if it
#       was not found. If no installation was found, the rest of the output
#       variables will be empty.
# - BOX2D_LIB:
#       Path to the Box2D library.
# - BOX2D_INCLUDE_DIR:
#		Directory containing the header files for Box2D.


find_library(BOX2D_LIB
	NAMES box2d libbox2d
)

find_path(BOX2D_INCLUDE_DIR
	NAMES Box2D.h
	PATH_SUFFIXES Box2D
)

string(FIND ${BOX2D_LIB} "-NOTFOUND" LIBRARY_FOUND)
string(FIND ${BOX2D_INCLUDE_DIR} "-NOTFOUND" HEADERS_FOUND)
if(${LIBRARY_FOUND} LESS 0 AND ${HEADERS_FOUND} LESS 0)
	set(BOX2D_FOUND TRUE)
	message(STATUS "Found box2d!")
else()
	set(BOX2D_FOUND FALSE)
	message(WARNING "Could NOT find box2d!")
endif()

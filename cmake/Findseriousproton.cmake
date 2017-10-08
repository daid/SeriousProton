#Finds an installation of Serious Proton on your system.
#
#Usage:
# find_package(SeriousProton)
#
#Output:
# The following variables will be defined by this script:
# - SERIOUS_PROTON_FOUND:
#       Set to TRUE when an installation of Serious Proton was found, or to
#       FALSE if it was not found. If no installation was found, the rest of
#       the output variables will be empty.
# - SERIOUS_PROTON_LIB:
#       Path to the Serious Proton library.
# - SERIOUS_PROTON_INCLUDE_DIR:
#		Directory containing the header files for Serious Proton.


find_library(SERIOUS_PROTON_LIB
	NAMES seriousproton libseriousproton
)

find_path(SERIOUS_PROTON_INCLUDE_DIR
	NAMES multiplayer_server_scanner.h
	PATH_SUFFIXES seriousproton
)

string(FIND ${SERIOUS_PROTON_LIB} "-NOTFOUND" LIBRARY_FOUND)
string(FIND ${SERIOUS_PROTON_INCLUDE_DIR} "-NOTFOUND" HEADERS_FOUND)
if(${LIBRARY_FOUND} LESS 0 AND ${HEADERS_FOUND} LESS 0)
	set(SERIOUS_PROTON_FOUND TRUE)
	message(STATUS "Found seriousproton!")
else()
	set(SERIOUS_PROTON_FOUND FALSE)
	message(WARNING "Could NOT find seriousproton!")
endif()

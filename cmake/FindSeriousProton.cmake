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

set(find_serious_proton_paths
	~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

find_library(SERIOUS_PROTON_LIB
	NAMES seriousproton
	PATH_SUFFIXES lib
	PATHS ${find_serious_proton_paths}
)

find_path(SERIOUS_PROTON_INCLUDE_DIR
	NAMES multiplayer_server_scanner.h
	PATH_SUFFIXES src
	PATHS ${find_serious_proton_paths}
)

if(${SERIOUS_PROTON_LIB})
	set(SERIOUS_PROTON_FOUND TRUE)
	message(STATUS "Found SeriousProton!")
else()
	set(SERIOUS_PROTON_FOUND FALSE)
	message(WARNING "Could NOT find SeriousProton!")
endif()

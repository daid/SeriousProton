# This "FindJson11.cmake" file has been taken from the following project:
#   https://github.com/MASKOR/mapit/blob/master/cmake/Findjson11.cmake
# which is licensed under LGPL-3.0 License
# Aditional changes were done by Michal Schorm

# Locate json11
#
# This module defines
#  json11_FOUND, if false, do not try to link to json11
#  json11_LIBRARY, where to find json11
#  json11_INCLUDE_DIR, where to find json11.hpp
#
# If json11 is not installed in a standard path, you can use the json11_DIR CMake variable
# to tell CMake where json11 is.

find_path(json11_INCLUDE_DIR json11.hpp
          PATHS
          /usr/local/include/
          /usr/include/
          /sw/json11/         # Fink
          /opt/local/json11/  # DarwinPorts
          /opt/csw/json11/    # Blastwave
          /opt/json11/
          /include/)

if(NOT WIN32)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
endif(NOT WIN32)

find_library(json11_LIBRARY
             NAMES  json11
             PATH_SUFFIXES lib64 lib
             PATHS  /usr/local
                    /usr
                    /sw
                    /opt/local
                    /opt/csw
                    /opt
                    /lib)

# handle the QUIETLY and REQUIRED arguments and set json11_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(json11 DEFAULT_MSG json11_INCLUDE_DIR json11_LIBRARY)
mark_as_advanced(json11_INCLUDE_DIR json11_LIBRARY)

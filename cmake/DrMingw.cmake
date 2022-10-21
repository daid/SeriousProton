# Crash Logger for MinGW
if(WIN32)
    option(ENABLE_CRASH_LOGGER "Enable the Dr. MinGW crash logging facilities" OFF)
    set(DRMINGW_ROOT DRMINGW_ROOT-NOTFOUND CACHE PATH "Path to Dr. MinGW")

    if(NOT ENABLE_CRASH_LOGGER)
        message(STATUS "Crash Logger is OFF")
    else()
        message(STATUS "Crash Logger is ON")

        if(NOT DRMINGW_ROOT)
            message(VERBOSE "Downloading Dr. MinGW")

            if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
                set(DRMINGW_ARCH "32")
            else()
                set(DRMINGW_ARCH "64")
            endif()

            # 0.9.x seems to give a hard time to people on Win7.
            # Sticking with 0.8 for that reason.
            set(DRMINGW_VERSION "0.8.2")
            set(DRMINGW_BASENAME "drmingw-${DRMINGW_VERSION}-win${DRMINGW_ARCH}")
            set(DRMINGW_ROOT "${CMAKE_CURRENT_BINARY_DIR}/${DRMINGW_BASENAME}" CACHE PATH "Path to Dr. MinGW" FORCE)

            if(NOT EXISTS "${DRMINGW_ROOT}/bin/exchndl.dll")
                set(DRMINGW_ZIP "${CMAKE_CURRENT_BINARY_DIR}/${DRMINGW_BASENAME}.7z")

                file(DOWNLOAD "https://github.com/jrfonseca/drmingw/releases/download/${DRMINGW_VERSION}/${DRMINGW_BASENAME}.7z" "${DRMINGW_ZIP}" TIMEOUT 60 TLS_VERIFY ON)
                execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf "${DRMINGW_ZIP}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
            endif()
        endif()
    endif()
endif()
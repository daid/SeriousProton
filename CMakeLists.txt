cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(SeriousProton LANGUAGES CXX)

set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/
    ${CMAKE_MODULE_PATH}
)

# User-settings
option(SERIOUSPROTON_WITH_JSON "Use json library." OFF)

if(WIN32)
    option(ENABLE_CRASH_LOGGER "Enable the Dr. MinGW crash logging facilities" OFF)
    set(DRMINGW_ROOT DRMINGW_ROOT-NOTFOUND CACHE PATH "Path to Dr. MinGW")
endif()

# Preflight checks.
message(STATUS "Crash Logger is " ${ENABLE_CRASH_LOGGER})

if(ENABLE_CRASH_LOGGER)
    if(NOT DRMINGW_ROOT)
        message("Downloading Dr. MinGW")

        set(DRMINGW_ARCH "64")
        if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
            set(DRMINGW_ARCH "32")
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

#--------------------------------Dependencies----------------------------------
find_package(SFML 2.5 COMPONENTS system audio network window graphics)
if(NOT ${SFML_FOUND})
    message(STATUS "Couldn't find SFML. Building it from scratch.")
    
    set(SFML_ROOT "${CMAKE_CURRENT_BINARY_DIR}/SFML-prefix")

    include(ExternalProject)

    ExternalProject_Add(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.5.1 #Always build with this tag, so we're sure it is stable.
        GIT_SHALLOW 1 # Don't get the entire history
        CMAKE_ARGS
            -DBUILD_SHARED_LIBS:BOOL=$<BOOL:${WIN32}>
            -DCMAKE_INSTALL_PREFIX:PATH=${SFML_ROOT}
            -DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    )

    set(SFML_INCLUDE_DIR "${SFML_ROOT}/include") #Otherwise this is filled by FindSFML.cmake.

    set(_SFML_LIB_PREFIX STATIC)
    set(_SFML_LIB_SUFFIX STATIC)

    if(WIN32)
        set(_SFML_LIB_PREFIX SHARED)
        set(_SFML_LIB_SUFFIX LINK)
    endif()

    set(SFML_LIBRARIES
        "-L${SFML_ROOT}/lib"
        "${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-audio${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-graphics${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-network${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-system${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-window${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
    )
endif()

add_subdirectory(src/Box2D)
add_subdirectory(src/lua)
add_subdirectory(src/GL)
add_subdirectory(src/libopus)

if(SERIOUSPROTON_WITH_JSON)
    add_subdirectory(src/json11)
endif()

#---------------------------------File lists-----------------------------------
set(source_files #All SeriousProton's objects to compile
    src/clipboard.cpp
    src/collisionable.cpp
    src/engine.cpp
    src/event.cpp
    src/fixedSocket.cpp
    src/gameEntity.cpp
    src/httpServer.cpp
    src/i18n.cpp
    src/input.cpp
    src/logging.cpp
    src/multiplayer.cpp
    src/multiplayer_client.cpp
    src/multiplayer_proxy.cpp
    src/multiplayer_server.cpp
    src/multiplayer_server_scanner.cpp
    src/networkAudioStream.cpp
    src/networkRecorder.cpp
    src/P.cpp
    src/PlayerController.cpp
    src/postProcessManager.cpp
    src/random.cpp
    src/Renderable.cpp
    src/resources.cpp
    src/scriptInterface.cpp
    src/scriptInterfaceMagic.cpp
    src/shaderManager.cpp
    src/soundManager.cpp
    src/stringImproved.cpp
    src/textureManager.cpp
    src/tween.cpp
    src/Updatable.cpp
    src/windowManager.cpp
)

if(NOT ANDROID)
    list(APPEND source_files src/dynamicLibrary.cpp)
endif()

#----------------------------------Compiling-----------------------------------


# Set our optimization flags.
set(OPTIMIZER_FLAGS)
if(CMAKE_COMPILER_IS_GNUCC)
    # On gcc, we want some general optimalizations that improve speed a lot.
    set(OPTIMIZER_FLAGS ${OPTIMIZER_FLAGS} -O3 -flto -funsafe-math-optimizations)

    # If we are compiling for a rasberry pi, we want to aggressively optimize for the CPU we are running on.
    # Note that this check only works if we are compiling directly on the pi, as it is a dirty way of checkif if we are on the pi.
    if(EXISTS /opt/vc/include/bcm_host.h OR COMPILE_FOR_PI)
        set(OPTIMIZER_FLAGS ${OPTIMIZER_FLAGS} -mcpu=native -mfpu=neon-vfpv4 -mfloat-abi=hard)
    endif()
endif()

# Targets setup
# =============
#
# * seriousproton - this is the consumer library, the one you use in a target_link_libraries() call as a consumer.
#
# * seriousproton_objects: SP source files.
#   MinGW has... troubles when LTO is enabled and linking a static library: https://stackoverflow.com/q/27372667
#   To work around that, sources are exposed to the consumer (through the interface library) and built in 'their' space.
#
# * seriousproton_deps: SP dependencies / setup (include dirs, linked libraries etc)
#
# Both seriousproton_objects and seriousproton 'link' against seriousproton_deps:
# This allows compile flags/settings to be forwarded to consumer, as well as proper configuration to compile individual SP objects.

## Common settings / dependencies (c++ standards, headers, dependencies etc)
add_library(seriousproton_deps INTERFACE)

target_include_directories(seriousproton_deps
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src;${SFML_INCLUDE_DIR}>"
)
target_compile_features(seriousproton_deps INTERFACE cxx_std_17)

if(NOT ${SFML_FOUND})
    add_dependencies(seriousproton_deps SFML)
endif()

target_compile_definitions(seriousproton_deps
    INTERFACE
        SFML_NO_DEPRECATED_WARNINGS # SFML 2.5 gives deprication warnings on a few functions we use. But we maintain 2.3 compatibility, so ignore those warnings.
        $<$<BOOL:${MSVC}>:NOMINMAX>
        $<$<CONFIG:Debug>:DEBUG>
        # Windows: Backwards compatibility with Win7 SP1: https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
        $<$<BOOL:${WIN32}>:WINVER=0x0601;_WIN32_WINNT=0x0601>
)

target_compile_options(seriousproton_deps
    INTERFACE
        "$<$<AND:$<NOT:$<BOOL:${MSVC}>>,$<CONFIG:RelWithDebInfo>>:-g1>"
        "$<$<AND:$<BOOL:${CMAKE_COMPILER_IS_GNUCC}>,$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:${OPTIMIZER_FLAGS}>"
        "$<$<NOT:$<BOOL:${MSVC}>>:-Wall;-Werror=return-type>"
        "$<$<BOOL:${MSVC}>:/MP;/permissive->"
)

target_link_libraries(seriousproton_deps
    INTERFACE 
        box2d lua glew ${SFML_LIBRARIES}
        "$<$<BOOL:${WIN32}>:wsock32>"
        # LTO flag must be on the linker's list as well.
        "$<$<AND:$<BOOL:${CMAKE_COMPILER_IS_GNUCC}>,$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:-flto>"
        "$<BUILD_INTERFACE:opus;$<$<BOOL:${SERIOUSPROTON_WITH_JSON}>:json11>;$<$<NOT:$<BOOL:${ANDROID}>>:${CMAKE_DL_LIBS}>>"
)

if(NOT ANDROID)
    # Necessary for some older compilers (except on android, where the fs api isn't used)
    if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
        target_link_libraries(seriousproton_deps INTERFACE $<BUILD_INTERFACE:stdc++fs>)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(seriousproton_deps INTERFACE $<BUILD_INTERFACE:c++fs>)
    endif()
endif()

## Object list
add_library(seriousproton_objects OBJECT ${source_files})

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
    # target_link_libraries() support for object libraries only exists since 3.12.
    target_link_libraries(seriousproton_objects PUBLIC $<BUILD_INTERFACE:seriousproton_deps>)
else()
    # This is mainly for compatibility with Ubuntu 18.04, which still uses CMake 3.10.
    set_target_properties(seriousproton_objects PROPERTIES LINK_LIBRARIES $<BUILD_INTERFACE:seriousproton_deps>)
endif()

## Public libraries that 'consumers' link against.
add_library(seriousproton INTERFACE)

# Expose sources to consumer (necessary for LTO on some compilers, mingw)
set_target_properties(seriousproton PROPERTIES INTERFACE_SOURCES $<TARGET_OBJECTS:seriousproton_objects>)

# Forward SP settings to consumer.
target_link_libraries(seriousproton INTERFACE $<BUILD_INTERFACE:seriousproton_deps>)

#--------------------------------Installation----------------------------------
install(
    TARGETS seriousproton
    EXPORT seriousproton
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(
    DIRECTORY ${seriousproton_include_dir}
    DESTINATION include/seriousproton
    FILES_MATCHING PATTERN "*.h"
    
)
install(
    DIRECTORY ${seriousproton_include_dir}
    DESTINATION include/seriousproton
    FILES_MATCHING PATTERN "*.hpp"
)
install(
    EXPORT seriousproton
    DESTINATION share/seriousproton
)

if(WIN32)
    install(
        FILES
            "${SFML_ROOT}/bin/OpenAL32.dll"
            "${SFML_ROOT}/bin/sfml-audio-2.dll"
            "${SFML_ROOT}/bin/sfml-graphics-2.dll"
            "${SFML_ROOT}/bin/sfml-network-2.dll"
            "${SFML_ROOT}/bin/sfml-system-2.dll"
            "${SFML_ROOT}/bin/sfml-window-2.dll"
        DESTINATION .
        COMPONENT runtime
    )

    if (MINGW)
        macro(get_mingw_dll dll location)
            execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${dll} OUTPUT_VARIABLE ${location} OUTPUT_STRIP_TRAILING_WHITESPACE)
        endmacro()
        
        # In the wild, MinGW come in different flavor
        # (Nuwen is fully statically linked, exception handlers varies sjlj / seh / dw2, threading model can be posix or windows, ...)
        # Grab anything possible, the correct ones will stick for the given installation.
        get_mingw_dll(libstdc++-6.dll MINGW_STDCPP_DLL)
        get_mingw_dll(libgcc_s_sjlj-1.dll MINGW_LIBGCCSJLJ_DLL)
        get_mingw_dll(libgcc_s_seh-1.dll MINGW_LIBGCCSEH_DLL)
        get_mingw_dll(libgcc_s_dw2-1.dll MINGW_LIBGCCDW2_DLL)
        get_mingw_dll(libwinpthread-1.dll MINGW_PTHREAD_DLL)

        install(
            FILES
                ${MINGW_STDCPP_DLL}
                ${MINGW_LIBGCCSJLJ_DLL}
                ${MINGW_LIBGCCSEH_DLL}
                ${MINGW_LIBGCCDW2_DLL}
                ${MINGW_PTHREAD_DLL}
            DESTINATION .
            COMPONENT runtime
        OPTIONAL)
    endif()

    if(ENABLE_CRASH_LOGGER)
        install(
            FILES
            ${DRMINGW_ROOT}/bin/dbghelp.dll
            ${DRMINGW_ROOT}/bin/exchndl.dll
            ${DRMINGW_ROOT}/bin/mgwhelp.dll
            ${DRMINGW_ROOT}/bin/symsrv.dll
            ${DRMINGW_ROOT}/bin/symsrv.yes
            DESTINATION .
            COMPONENT runtime
        )
    endif()
endif()

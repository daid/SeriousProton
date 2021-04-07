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
find_package(SFML 2.4 COMPONENTS system audio network window graphics)
if(NOT ${SFML_FOUND})
    message(STATUS "Couldn't find SFML. Building it from scratch.")
    
    set(SFML_ROOT "${CMAKE_CURRENT_BINARY_DIR}/SFML-prefix")

    include(ExternalProject)

    ExternalProject_Add(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.5.1 #Always build with this tag, so we're sure it is stable.
        GIT_SHALLOW 1 # Don't get the entire history
        CMAKE_ARGS -DBUILD_SHARED_LIBS=$<BOOL:${WIN32}> -DCMAKE_INSTALL_PREFIX=${SFML_ROOT}
    )

    set(SFML_INCLUDE_DIR "${SFML_ROOT}/include") #Otherwise this is filled by FindSFML.cmake.

    set(_SFML_LIB_PREFIX STATIC)
    set(_SFML_LIB_SUFFIX STATIC)

    if(WIN32)
        set(_SFML_LIB_PREFIX SHARED)
        set(_SFML_LIB_SUFFIX LINK)
    endif()

    set(SFML_LIBRARIES
        "${SFML_ROOT}/lib/${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-audio${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${SFML_ROOT}/lib/${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-graphics${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${SFML_ROOT}/lib/${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-network${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${SFML_ROOT}/lib/${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-system${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
        "${SFML_ROOT}/lib/${CMAKE_${_SFML_LIB_TYPE}_LIBRARY_PREFIX}sfml-window${CMAKE_${_SFML_LIB_SUFFIX}_LIBRARY_SUFFIX}"
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

add_library(seriousproton STATIC ${source_files})
target_compile_features(seriousproton PUBLIC cxx_std_17)

if(NOT ${SFML_FOUND})
    add_dependencies(seriousproton SFML)
endif()

target_include_directories(seriousproton
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src;${SFML_INCLUDE_DIR}>"
)

target_compile_definitions(seriousproton
    PUBLIC
        SFML_NO_DEPRECATED_WARNINGS # SFML 2.5 gives deprication warnings on a few functions we use. But we maintain 2.3 compatibility, so ignore those warnings.
        $<$<BOOL:${MSVC}>:NOMINMAX>
        $<$<CONFIG:Debug>:DEBUG>
        # Windows: Backwards compatibility with Win7 SP1: https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
        $<$<BOOL:${WIN32}>:WINVER=0x0601;_WIN32_WINNT=0x0601>
)

target_compile_options(seriousproton
    PUBLIC
        "$<$<AND:$<NOT:$<BOOL:${MSVC}>>,$<CONFIG:RelWithDebInfo>>:-g1>"
        "$<$<AND:$<NOT:$<BOOL:${MSVC}>>,$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:${OPTIMIZER_FLAGS}>"
        "$<$<NOT:$<BOOL:${MSVC}>>:-Wall;-Werror=return-type>"
        "$<$<BOOL:${MSVC}>:/MP;/permissive->"
)

target_link_libraries(seriousproton
    PUBLIC 
        box2d lua glew ${SFML_LIBRARIES}
        "$<$<BOOL:${WIN32}>:wsock32>"
    PRIVATE $<BUILD_INTERFACE:opus>
    INTERFACE
        $<BUILD_INTERFACE:$<$<BOOL:${SERIOUSPROTON_WITH_JSON}>:json11>>
)

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
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++-6.dll OUTPUT_VARIABLE MINGW_STDCPP_DLL OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libgcc_s_sjlj-1.dll OUTPUT_VARIABLE MINGW_LIBGCC_DLL OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libwinpthread-1.dll OUTPUT_VARIABLE MINGW_PTHREAD_DLL OUTPUT_STRIP_TRAILING_WHITESPACE)
        
        install(
            FILES
                ${MINGW_STDCPP_DLL}
                ${MINGW_LIBGCC_DLL}
                ${MINGW_PTHREAD_DLL}
            DESTINATION .
            COMPONENT runtime
        )
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

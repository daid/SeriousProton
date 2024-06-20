cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(SeriousProton LANGUAGES C CXX)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
    ${CMAKE_MODULE_PATH}
)
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# User-settings
option(WARNING_IS_ERROR "Enable warning as errors." OFF)
option(SHARED_SP "Build SeriousProton as a shared library, to speed up mingw linking times" OFF)
set(STEAMSDK "" CACHE PATH "Path to steam SDK, if not supplied steam features will not be available. Steam features are NOT required.")

#
set(EXTERNALS_DIR "${PROJECT_BINARY_DIR}/externals")
set(DOWNLOADS_DIR "${PROJECT_BINARY_DIR}/downloads")
file(MAKE_DIRECTORY "${EXTERNAL_DIR}" "${DOWNLOADS_DIR}")

# We just want C++17 for everything we build
set(CMAKE_CXX_STANDARD 17)

include("cmake/DrMingw.cmake")

#--------------------------------Dependencies----------------------------------
find_package(SDL2 REQUIRED)
if(NOT DEFINED SDL2_LIBRARIES)
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main)
endif()

# GLM - OpenGL Mathematic library
SET(WITH_GLM "auto" CACHE STRING "Which GLM to use (possible values are 'bundled', 'system' or 'auto')")

if(NOT WITH_GLM STREQUAL "bundled")
    set(glm_find_args QUIET)

    if(WITH_GLM STREQUAL "system")
       set(glm_find_args REQUIRED)
    endif()

    # Not bundled - attempt to find system package first.
    find_package(glm ${glm_find_args})
endif()

if(glm_FOUND)
    message(STATUS "GLM version used: SYSTEM")
    if(NOT TARGET glm::glm)
        # Compatibility with older versions
        add_library(glm::glm ALIAS glm)
    endif()
else()
    # We can only be coming here from:
    #    * 'auto', if we failed to find a suitable system package,
    #    * 'bundled'
    message(STATUS "GLM version used: BUNDLED")
    
    set(GLM_VERSION "0.9.9.8")
    set(GLM_URL "https://github.com/g-truc/glm")
	include(FetchContent)
	FetchContent_Declare(
		glm
		GIT_REPOSITORY "${GLM_URL}"
		GIT_TAG "${GLM_VERSION}"
	)
	FetchContent_GetProperties(glm)

    if(NOT glm_POPULATED)
        if(COMMAND FetchContent_Populate)
            FetchContent_Populate(glm)
        endif()
        add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

add_subdirectory(libs/Box2D)
add_subdirectory(libs/glad)

# See cmake/GenerateGlDebug.cmake for details.
get_target_property(glad_source_dir glad SOURCE_DIR)
set(glDebug_inl "${CMAKE_CURRENT_BINARY_DIR}/include/graphics/glDebug.inl")
add_custom_command(
    OUTPUT "${glDebug_inl}"
    COMMAND ${CMAKE_COMMAND} -Dglad_h="${glad_source_dir}/glad/glad.h" -DglDebug_inl="${glDebug_inl}" -P cmake/GenerateGlDebug.cmake
    MAIN_DEPENDENCY "${glad_source_dir}/glad/glad.h"
    DEPENDS cmake/glDebug.inl.in cmake/GenerateGlDebug.cmake
    COMMENT "Generating glDebug.inl"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_source_files_properties("${glDebug_inl}" PROPERTIES GENERATED TRUE)

add_subdirectory(libs/lua)

SET(WITH_OPUS "bundled" CACHE STRING "Which opus library to use (possible values are 'bundled' or 'system')")

if(WITH_OPUS STREQUAL "system")
    find_package(PkgConfig)
    pkg_check_modules(Opus REQUIRED opus IMPORTED_TARGET)
    message(STATUS "opus version used: SYSTEM")
    add_library(opus ALIAS PkgConfig::Opus)
elseif(WITH_OPUS STREQUAL "bundled")
    message(STATUS "opus version used: BUNDLED")
    add_subdirectory(libs/libopus)
endif()

# FreeType 2
set(WITH_FREETYPE2 "auto" CACHE STRING "Which FreeType2 to use (possible values are 'bundled', 'system' or 'auto')")

if(NOT WITH_FREETYPE2 STREQUAL "bundled")

    set(freetype2_find_args QUIET)

    if(WITH_FREETYPE2 STREQUAL "system")
       set(freetype2_find_args REQUIRED)
    endif()

    # Not bundled - attempt to find system package first.
    find_package(Freetype 2 ${freetype2_find_args})
endif()

if(Freetype_FOUND)
    message(STATUS "Freetype 2 version used: SYSTEM")
else()
    # We can only be coming here from:
    #    * 'auto', if we failed to find a suitable system package,
    #    * 'bundled'
    set(Freetype_DIR libs/cmake)
    find_package(Freetype 2 REQUIRED NO_DEFAULT_PATH)
    message(STATUS "Freetype 2 version used: BUNDLED")
endif()

# BASIS Universal (Supercompressed GPU Texture Codec)
add_subdirectory(libs/basis_universal)

#---------------------------------File lists-----------------------------------
set(source_files #All SeriousProton's objects to compile
    src/audio/source.cpp
    src/audio/sound.cpp
    src/audio/music.cpp
    src/clipboard.cpp
    src/collisionable.cpp
    src/engine.cpp
    src/event.cpp
    src/graphics/font.cpp
    src/graphics/freetypefont.cpp
    src/graphics/image.cpp
    src/graphics/ktx2texture.cpp
    src/graphics/renderTarget.cpp
    src/graphics/texture.cpp
    src/graphics/textureAtlas.cpp
    src/graphics/renderTexture.cpp
    src/graphics/shader.cpp
    src/graphics/opengl.cpp
    src/i18n.cpp
    src/keyValueTree.cpp
    src/logging.cpp
    src/multiplayer.cpp
    src/multiplayer_client.cpp
    src/multiplayer_proxy.cpp
    src/multiplayer_server.cpp
    src/multiplayer_server_scanner.cpp
    src/networkAudioStream.cpp
    src/networkRecorder.cpp
    src/P.cpp
    src/postProcessManager.cpp
    src/random.cpp
    src/Renderable.cpp
    src/resources.cpp
    src/scriptInterface.cpp
    src/scriptInterfaceMagic.cpp
    src/scriptInterfaceSandbox.cpp
    src/shaderManager.cpp
    src/soundManager.cpp
    src/stringImproved.cpp
    src/stringutil/base64.cpp
    src/stringutil/sha1.cpp
    src/textureManager.cpp
    src/timer.cpp
    src/tween.cpp
    src/Updatable.cpp
    src/windowManager.cpp
    src/io/keybinding.cpp
    src/io/keyValueTreeLoader.cpp
    src/io/network/address.cpp
    src/io/network/selector.cpp
    src/io/network/socketBase.cpp
    src/io/network/tcpListener.cpp
    src/io/network/streamSocket.cpp
    src/io/network/tcpSocket.cpp
    src/io/network/udpSocket.cpp
    src/io/http/request.cpp
    src/io/http/server.cpp
    src/io/http/websocket.cpp

    src/audio/source.h
    src/audio/sound.h
    src/audio/music.h
    src/clipboard.h
    src/collisionable.h
    src/dynamicLibrary.h
    src/engine.h
    src/event.h
    src/graphics/alignment.h
    src/graphics/font.h
    src/graphics/freetypefont.h
    src/graphics/image.h
    src/graphics/ktx2texture.h
    src/graphics/renderTarget.h
    src/graphics/texture.h
    src/graphics/textureAtlas.h
    src/graphics/renderTexture.h
    src/graphics/shader.h
    src/graphics/opengl.h
    src/i18n.h
    src/keyValueTree.h
    
    src/io/dataBuffer.h
    src/io/json.h
    src/io/keybinding.h
    src/io/keyValueTreeLoader.h
    src/io/pointer.h
    src/io/textinput.h
    src/io/http/request.h
    src/io/http/server.h
    src/io/http/websocket.h
    src/io/network/address.h
    src/io/network/selector.h
    src/io/network/socketBase.h
    src/io/network/tcpListener.h
    src/io/network/streamSocket.h
    src/io/network/tcpSocket.h
    src/io/network/udpSocket.h
    src/logging.h
    src/multiplayer_client.h
    src/multiplayer.h
    src/multiplayer_internal.h
    src/multiplayer_proxy.h
    src/multiplayer_server.h
    src/multiplayer_server_scanner.h
    src/networkAudioStream.h
    src/networkRecorder.h
    src/nonCopyable.h
    src/P.h
    src/postProcessManager.h
    src/random.h
    src/rect.h
    src/Renderable.h
    src/resources.h
    src/scriptInterface.h
    src/scriptInterfaceMagic.h
    src/scriptInterfaceSandbox.h
    src/shaderManager.h
    src/soundManager.h
    src/stringImproved.h
    src/stringutil/base64.h
    src/stringutil/sha1.h
    src/textureManager.h
    src/tween.h
    src/timer.h
    src/Updatable.h
    src/vectorUtils.h
    src/windowManager.h

    cmake/glDebug.inl.in
    "${glDebug_inl}"
)

if(STEAMSDK)
    list(APPEND source_files
        src/io/network/steamP2PListener.cpp
        src/io/network/steamP2PListener.h
        src/io/network/steamP2PSocket.cpp
        src/io/network/steamP2PSocket.h
    )
endif()

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
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src;${CMAKE_CURRENT_BINARY_DIR}/include;${SDL2_INCLUDE_DIRS};${FREETYPE_INCLUDE_DIRS}>"
)
target_compile_features(seriousproton_deps INTERFACE cxx_std_17)

find_package(Threads REQUIRED)

target_compile_definitions(seriousproton_deps
    INTERFACE
        $<$<BOOL:${MSVC}>:NOMINMAX;_CRT_SECURE_NO_WARNINGS>
        $<$<CONFIG:Debug>:DEBUG>
        # Windows: Backwards compatibility with Win7 SP1: https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
        $<$<BOOL:${WIN32}>:WINVER=0x0601;_WIN32_WINNT=0x0601>
        $<$<AND:$<PLATFORM_ID:Android>,$<NOT:$<CONFIG:Release>>>:NDK_DEBUG=1>
)

target_compile_options(seriousproton_deps
    INTERFACE
        "$<$<AND:$<NOT:$<BOOL:${MSVC}>>,$<CONFIG:RelWithDebInfo>>:-g1>"
        "$<$<AND:$<BOOL:${CMAKE_COMPILER_IS_GNUCC}>,$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:${OPTIMIZER_FLAGS}>"
        "$<$<NOT:$<BOOL:${MSVC}>>:-Wall;-Wextra;-Woverloaded-virtual;-Wdouble-promotion;-Wsuggest-override;-Werror=return-type;-Wno-unused-parameter;-Wno-unused-but-set-parameter>"
        "$<$<BOOL:${MSVC}>:/MP;/permissive->"
)

include("cmake/SteamSDK.cmake")
target_link_libraries(seriousproton_deps
    INTERFACE 
        box2d glad lua ${SDL2_LIBRARIES} glm::glm Threads::Threads ${FREETYPE_LIBRARIES} basisu-transcoder $<BUILD_INTERFACE:steamsdk>
        $<$<BOOL:${WIN32}>:wsock32 ws2_32 crypt32 iphlpapi>
        # LTO flag must be on the linker's list as well.
        "$<$<AND:$<BOOL:${CMAKE_COMPILER_IS_GNUCC}>,$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>>:-flto>"
        "$<BUILD_INTERFACE:opus;$<$<NOT:$<BOOL:${ANDROID}>>:${CMAKE_DL_LIBS}>>"
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
if (SHARED_SP)
	add_library(seriousproton_objects SHARED ${source_files})
else()
	add_library(seriousproton_objects OBJECT ${source_files})
endif()
target_compile_options(seriousproton_objects
    PRIVATE
        "$<$<AND:$<BOOL:${CMAKE_COMPILER_IS_GNUCC}>,$<BOOL:${WARNING_IS_ERROR}>>:-Werror>"
        "$<$<AND:$<BOOL:${MSVC}>,$<BOOL:${WARNING_IS_ERROR}>>:/WX>"
)

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
    if(MINGW)
        # For the MinGW library, the 'imported location' looks at the dll in the wrong folder (lib instead of bin)
        # So we have to get it by hand.
        # Also covers backwards compatibility with SDL version < 2.0.12.
        install(FILES "${SDL2_PREFIX}/bin/SDL2.dll" DESTINATION . COMPONENT runtime)
    else()
        install(PROGRAMS "$<TARGET_PROPERTY:SDL2::SDL2,IMPORTED_LOCATION>" DESTINATION . COMPONENT runtime)
    endif()
    if(STEAMSDK)
        if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
            install(FILES "${STEAMSDK}/redistributable_bin/steam_api.dll" DESTINATION . COMPONENT runtime)
        else()
            install(FILES "${STEAMSDK}/redistributable_bin/win64/steam_api64.dll" DESTINATION . COMPONENT runtime)
        endif()
    endif()

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
if(UNIX AND STEAMSDK)
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        install(FILES "${STEAMSDK}/redistributable_bin/linux32/libsteam_api.so" DESTINATION . COMPONENT runtime)
    else()
        install(FILES "${STEAMSDK}/redistributable_bin/linux64/libsteam_api.so" DESTINATION . COMPONENT runtime)
    endif()
endif()

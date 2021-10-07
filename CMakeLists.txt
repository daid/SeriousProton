cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
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
option(SERIOUSPROTON_WITH_JSON "Use json library." OFF)

#
set(EXTERNALS_DIR "${PROJECT_BINARY_DIR}/externals")
set(DOWNLOADS_DIR "${PROJECT_BINARY_DIR}/downloads")
file(MAKE_DIRECTORY "${EXTERNAL_DIR}" "${DOWNLOADS_DIR}")

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

#--------------------------------Dependencies----------------------------------
find_package(SDL2 REQUIRED)
if(NOT DEFINED SDL2_LIBRARIES)
    set(SDL2_LIBRARIES SDL2::SDL2)
endif()

# GLM - OpenGL Mathematic library
SET(WITH_GLM "auto" CACHE STRING "Which GLM to use (possible values are 'bundled', 'system' or 'auto')")

if(WITH_GLM STREQUAL "system" OR WITH_GLM STREQUAL "auto")
    find_package(glm QUIET)

    if(glm_FOUND)
        message(STATUS "GLM version used: SYSTEM")
    elseif(WITH_GLM STREQUAL "system")
        message(FATAL_ERROR "GLM: system package is not found or is unusable")
    endif()
endif()

if(WITH_GLM STREQUAL "bundled" OR (WITH_GLM STREQUAL "auto" AND NOT glm_FOUND))
    message(STATUS "GLM version used: BUNDLED")

    set(GLM_BASE_PATH "${EXTERNALS_DIR}")

    if(NOT EXISTS "${GLM_BASE_PATH}/glm/CMakeLists.txt")
        set(GLM_ZIP "${DOWNLOADS_DIR}/glm.zip")
        file(DOWNLOAD "https://github.com/g-truc/glm/releases/download/0.9.9.8/glm-0.9.9.8.zip" "${GLM_ZIP}" TIMEOUT 60 TLS_VERIFY ON)

        file(MAKE_DIRECTORY "${GLM_BASE_PATH}/glm")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf "${GLM_ZIP}" WORKING_DIRECTORY "${GLM_BASE_PATH}")
    endif()

    add_subdirectory("${GLM_BASE_PATH}/glm" "${PROJECT_BINARY_DIR}/glm" EXCLUDE_FROM_ALL)
endif()

add_subdirectory(libs/Box2D)
add_subdirectory(libs/lua)

# json11
if(SERIOUSPROTON_WITH_JSON)
    SET(WITH_JSON "bundled" CACHE STRING "Which json11 to use (possible values are 'bundled' or 'system')")

    if(WITH_JSON STREQUAL "system")
        find_package(json11 QUIET)
        if(json11_FOUND)
            message(STATUS "json11 version used: SYSTEM")
        else()
            message(FATAL_ERROR "json11: system package is not found or is unusable")
        endif()
    elseif(WITH_JSON STREQUAL "bundled")
        message(STATUS "json11 version used: BUNDLED")
        add_subdirectory(libs/json11)
    endif()
endif()

SET(WITH_OPUS "bundled" CACHE STRING "Which opus library to use (possible values are 'bundled' or 'system')")

if(WITH_OPUS STREQUAL "system")
    find_package(PkgConfig)
    pkg_check_modules(OPUS REQUIRED opus)
    message(STATUS "opus version used: SYSTEM")
    add_library(opus INTERFACE)
    target_link_libraries(opus ${OPUS_LIBRARIES})
    target_include_directories(opus PUBLIC ${OPUS_INCLUDE_DIRS})
    target_compile_options(opus PUBLIC ${OPUS_CFLAGS_OTHER})
elseif(WITH_OPUS STREQUAL "bundled")
    message(STATUS "opus version used: BUNDLED")
    add_subdirectory(libs/libopus)
endif()

add_library(spfreetype2 STATIC
    libs/freetype2/src/autofit/autofit.c
        #libs/freetype2/src/autofit/afblue.c
        #libs/freetype2/src/autofit/afcjk.c
        #libs/freetype2/src/autofit/afdummy.c
        #libs/freetype2/src/autofit/afglobal.c
        #libs/freetype2/src/autofit/afhints.c
        #libs/freetype2/src/autofit/afindic.c
        #libs/freetype2/src/autofit/aflatin.c
        #libs/freetype2/src/autofit/afloader.c
        #libs/freetype2/src/autofit/afmodule.c
        #libs/freetype2/src/autofit/afranges.c
        #libs/freetype2/src/autofit/afshaper.c
    libs/freetype2/src/base/ftbase.c
        #libs/freetype2/src/base/ftadvanc.c
        #libs/freetype2/src/base/ftcalc.c
        #libs/freetype2/src/base/ftcolor.c
        #libs/freetype2/src/base/ftdbgmem.c
        #libs/freetype2/src/base/fterrors.c
        #libs/freetype2/src/base/ftfntfmt.c
        #libs/freetype2/src/base/ftgloadr.c
        #libs/freetype2/src/base/fthash.c
        #libs/freetype2/src/base/ftlcdfil.c
        #libs/freetype2/src/base/ftmac.c
        #libs/freetype2/src/base/ftobjs.c
        #libs/freetype2/src/base/ftoutln.c
        #libs/freetype2/src/base/ftpsprop.c
        #libs/freetype2/src/base/ftrfork.c
        #libs/freetype2/src/base/ftsnames.c
        #libs/freetype2/src/base/ftstream.c
        #libs/freetype2/src/base/fttrigon.c
        #libs/freetype2/src/base/ftutil.c
    libs/freetype2/src/base/ftbbox.c
    libs/freetype2/src/base/ftbdf.c
    libs/freetype2/src/base/ftbitmap.c
    
    libs/freetype2/src/base/ftcid.c
    
    
    libs/freetype2/src/base/ftdebug.c
    
    
    libs/freetype2/src/base/ftfstype.c
    libs/freetype2/src/base/ftgasp.c
    
    libs/freetype2/src/base/ftglyph.c
    libs/freetype2/src/base/ftgxval.c
    
    libs/freetype2/src/base/ftinit.c
    
    
    libs/freetype2/src/base/ftmm.c
    
    libs/freetype2/src/base/ftotval.c
    
    libs/freetype2/src/base/ftpatent.c
    libs/freetype2/src/base/ftpfr.c
    
    
    
    
    libs/freetype2/src/base/ftstroke.c
    libs/freetype2/src/base/ftsynth.c
    libs/freetype2/src/base/ftsystem.c
    
    libs/freetype2/src/base/fttype1.c
    
    libs/freetype2/src/base/ftver.rc
    libs/freetype2/src/base/ftwinfnt.c
    libs/freetype2/src/base/md5.c
    libs/freetype2/src/bdf/bdf.c
        #libs/freetype2/src/bdf/bdfdrivr.c
        #libs/freetype2/src/bdf/bdflib.c
    libs/freetype2/src/bzip2/ftbzip2.c
    libs/freetype2/src/cache/ftcache.c
        #libs/freetype2/src/cache/ftcbasic.c
        #libs/freetype2/src/cache/ftccache.c
        #libs/freetype2/src/cache/ftccmap.c
        #libs/freetype2/src/cache/ftcglyph.c
        #libs/freetype2/src/cache/ftcimage.c
        #libs/freetype2/src/cache/ftcmanag.c
        #libs/freetype2/src/cache/ftcmru.c
        #libs/freetype2/src/cache/ftcsbits.c
    libs/freetype2/src/cff/cff.c
        #libs/freetype2/src/cff/cffcmap.c
        #libs/freetype2/src/cff/cffdrivr.c
        #libs/freetype2/src/cff/cffgload.c
        #libs/freetype2/src/cff/cffload.c
        #libs/freetype2/src/cff/cffobjs.c
        #libs/freetype2/src/cff/cffparse.c
    libs/freetype2/src/cid/type1cid.c
        #libs/freetype2/src/cid/cidgload.c
        #libs/freetype2/src/cid/cidload.c
        #libs/freetype2/src/cid/cidobjs.c
        #libs/freetype2/src/cid/cidparse.c
        #libs/freetype2/src/cid/cidriver.c
    #libs/freetype2/src/dlg/dlg.c
    #libs/freetype2/src/dlg/dlgwrap.c
    libs/freetype2/src/gxvalid/gxvalid.c
        #libs/freetype2/src/gxvalid/gxvbsln.c
        #libs/freetype2/src/gxvalid/gxvcommn.c
        #libs/freetype2/src/gxvalid/gxvfeat.c
        #libs/freetype2/src/gxvalid/gxvjust.c
        #libs/freetype2/src/gxvalid/gxvkern.c
        #libs/freetype2/src/gxvalid/gxvlcar.c
        #libs/freetype2/src/gxvalid/gxvmod.c
        #libs/freetype2/src/gxvalid/gxvmort.c
        #libs/freetype2/src/gxvalid/gxvmort0.c
        #libs/freetype2/src/gxvalid/gxvmort1.c
        #libs/freetype2/src/gxvalid/gxvmort2.c
        #libs/freetype2/src/gxvalid/gxvmort4.c
        #libs/freetype2/src/gxvalid/gxvmort5.c
        #libs/freetype2/src/gxvalid/gxvmorx.c
        #libs/freetype2/src/gxvalid/gxvmorx0.c
        #libs/freetype2/src/gxvalid/gxvmorx1.c
        #libs/freetype2/src/gxvalid/gxvmorx2.c
        #libs/freetype2/src/gxvalid/gxvmorx4.c
        #libs/freetype2/src/gxvalid/gxvmorx5.c
        #libs/freetype2/src/gxvalid/gxvopbd.c
        #libs/freetype2/src/gxvalid/gxvprop.c
        #libs/freetype2/src/gxvalid/gxvtrak.c
    #libs/freetype2/src/gxvalid/gxvfgen.c
    libs/freetype2/src/gzip/ftgzip.c
        #libs/freetype2/src/gzip/adler32.c
        #libs/freetype2/src/gzip/infblock.c
        #libs/freetype2/src/gzip/infcodes.c
        #libs/freetype2/src/gzip/inflate.c
        #libs/freetype2/src/gzip/inftrees.c
        #libs/freetype2/src/gzip/infutil.c
        #libs/freetype2/src/gzip/zutil.c
    libs/freetype2/src/lzw/ftlzw.c
        #libs/freetype2/src/lzw/ftzopen.c
    libs/freetype2/src/otvalid/otvalid.c
        #libs/freetype2/src/otvalid/otvbase.c
        #libs/freetype2/src/otvalid/otvcommn.c
        #libs/freetype2/src/otvalid/otvgdef.c
        #libs/freetype2/src/otvalid/otvgpos.c
        #libs/freetype2/src/otvalid/otvgsub.c
        #libs/freetype2/src/otvalid/otvjstf.c
        #libs/freetype2/src/otvalid/otvmath.c
        #libs/freetype2/src/otvalid/otvmod.c
    libs/freetype2/src/pcf/pcf.c
        #libs/freetype2/src/pcf/pcfdrivr.c
        #libs/freetype2/src/pcf/pcfread.c
        #libs/freetype2/src/pcf/pcfutil.c
    libs/freetype2/src/pfr/pfr.c
        #libs/freetype2/src/pfr/pfrcmap.c
        #libs/freetype2/src/pfr/pfrdrivr.c
        #libs/freetype2/src/pfr/pfrgload.c
        #libs/freetype2/src/pfr/pfrload.c
        #libs/freetype2/src/pfr/pfrobjs.c
        #libs/freetype2/src/pfr/pfrsbit.c
    libs/freetype2/src/psaux/psaux.c
        #libs/freetype2/src/psaux/afmparse.c
        #libs/freetype2/src/psaux/psauxmod.c
        #libs/freetype2/src/psaux/psobjs.c
        #libs/freetype2/src/psaux/t1cmap.c
        #libs/freetype2/src/psaux/t1decode.c
        #libs/freetype2/src/psaux/cffdecode.c
        #libs/freetype2/src/psaux/psarrst.c
        #libs/freetype2/src/psaux/psblues.c
        #libs/freetype2/src/psaux/pserror.c
        #libs/freetype2/src/psaux/psfont.c
        #libs/freetype2/src/psaux/psft.c
        #libs/freetype2/src/psaux/pshints.c
        #libs/freetype2/src/psaux/psintrp.c
        #libs/freetype2/src/psaux/psread.c
        #libs/freetype2/src/psaux/psstack.c
    libs/freetype2/src/psaux/psconv.c
    libs/freetype2/src/pshinter/pshinter.c
        #libs/freetype2/src/pshinter/pshalgo.c
        #libs/freetype2/src/pshinter/pshglob.c
        #libs/freetype2/src/pshinter/pshmod.c
        #libs/freetype2/src/pshinter/pshrec.c
    libs/freetype2/src/psnames/psnames.c
        #libs/freetype2/src/psnames/psmodule.c
    libs/freetype2/src/raster/raster.c
        #libs/freetype2/src/raster/ftraster.c
        #libs/freetype2/src/raster/ftrend1.c
    libs/freetype2/src/sdf/sdf.c
        #libs/freetype2/src/sdf/ftsdfrend.c
        #libs/freetype2/src/sdf/ftsdfcommon.c
        #libs/freetype2/src/sdf/ftbsdf.c
        #libs/freetype2/src/sdf/ftsdf.c 
    libs/freetype2/src/sfnt/sfnt.c
        #libs/freetype2/src/sfnt/pngshim.c
        #libs/freetype2/src/sfnt/sfdriver.c
        #libs/freetype2/src/sfnt/sfobjs.c
        #libs/freetype2/src/sfnt/sfwoff.c
        #libs/freetype2/src/sfnt/sfwoff2.c
        #libs/freetype2/src/sfnt/ttbdf.c
        #libs/freetype2/src/sfnt/ttcmap.c
        #libs/freetype2/src/sfnt/ttcolr.c
        #libs/freetype2/src/sfnt/ttcpal.c
        #libs/freetype2/src/sfnt/ttkern.c
        #libs/freetype2/src/sfnt/ttload.c
        #libs/freetype2/src/sfnt/ttmtx.c
        #libs/freetype2/src/sfnt/ttpost.c
        #libs/freetype2/src/sfnt/ttsbit.c
        #libs/freetype2/src/sfnt/woff2tags.c
    
    libs/freetype2/src/smooth/smooth.c
        #libs/freetype2/src/smooth/ftgrays.c
        #libs/freetype2/src/smooth/ftsmooth.c
    
    #libs/freetype2/src/tools/apinames.c
    #libs/freetype2/src/tools/ftrandom/ftrandom.c
    #libs/freetype2/src/tools/test_afm.c
    #libs/freetype2/src/tools/test_bbox.c
    #libs/freetype2/src/tools/test_trig.c
    libs/freetype2/src/truetype/truetype.c
        #libs/freetype2/src/truetype/ttdriver.c
        #libs/freetype2/src/truetype/ttgload.c
        #libs/freetype2/src/truetype/ttgxvar.c
        #libs/freetype2/src/truetype/ttinterp.c
        #libs/freetype2/src/truetype/ttobjs.c
        #libs/freetype2/src/truetype/ttpload.c
        #libs/freetype2/src/truetype/ttsubpix.c
    libs/freetype2/src/type1/type1.c
        #libs/freetype2/src/type1/t1afm.c
        #libs/freetype2/src/type1/t1driver.c
        #libs/freetype2/src/type1/t1gload.c
        #libs/freetype2/src/type1/t1load.c
        #libs/freetype2/src/type1/t1objs.c
        #libs/freetype2/src/type1/t1parse.c
    libs/freetype2/src/type42/type42.c
        #libs/freetype2/src/type42/t42drivr.c
        #libs/freetype2/src/type42/t42objs.c
        #libs/freetype2/src/type42/t42parse.c
    libs/freetype2/src/winfonts/winfnt.c
)
target_include_directories(spfreetype2 PUBLIC libs/freetype2/include)
target_compile_definitions(spfreetype2 PRIVATE FT2_BUILD_LIBRARY)

#---------------------------------File lists-----------------------------------
set(source_files #All SeriousProton's objects to compile
    src/audio/source.cpp
    src/audio/sound.cpp
    src/audio/music.cpp
    src/clipboard.cpp
    src/collisionable.cpp
    src/engine.cpp
    src/event.cpp
    src/gameEntity.cpp
    src/graphics/font.cpp
    src/graphics/freetypefont.cpp
    src/graphics/image.cpp
    src/graphics/renderTarget.cpp
    src/graphics/texture.cpp
    src/graphics/textureAtlas.cpp
    src/graphics/shader.cpp
    src/graphics/opengl.cpp
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
    src/stringutil/base64.cpp
    src/stringutil/sha1.cpp
    src/textureManager.cpp
    src/timer.cpp
    src/tween.cpp
    src/Updatable.cpp
    src/windowManager.cpp
    src/io/network/address.cpp
    src/io/network/selector.cpp
    src/io/network/socketBase.cpp
    src/io/network/tcpListener.cpp
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
    src/gameEntity.h
    src/graphics/alignment.h
    src/graphics/font.h
    src/graphics/freetypefont.h
    src/graphics/image.h
    src/graphics/renderTarget.h
    src/graphics/texture.h
    src/graphics/textureAtlas.h
    src/graphics/shader.h
    src/graphics/opengl.h
    src/graphics/openglDesktop.h
    src/graphics/opengles2.h
    src/i18n.h
    src/input.h
    src/io/dataBuffer.h
    src/io/http/request.h
    src/io/http/server.h
    src/io/http/websocket.h
    src/io/network/address.h
    src/io/network/selector.h
    src/io/network/socketBase.h
    src/io/network/tcpListener.h
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
    src/PlayerController.h
    src/postProcessManager.h
    src/random.h
    src/rect.h
    src/Renderable.h
    src/resources.h
    src/scriptInterface.h
    src/scriptInterfaceMagic.h
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
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src;${SDL2_INCLUDE_DIRS}>"
)
target_compile_features(seriousproton_deps INTERFACE cxx_std_17)

find_package(Threads REQUIRED)

target_compile_definitions(seriousproton_deps
    INTERFACE
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
        box2d lua ${SDL2_LIBRARIES} glm::glm Threads::Threads spfreetype2
        $<$<BOOL:${WIN32}>:wsock32 ws2_32 crypt32>
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
    message(WARNING "SDL2.dll install missing for package")
    #install(
    #    FILES
    #        "${SFML_ROOT}/bin/OpenAL32.dll"
    #        "${SFML_ROOT}/bin/sfml-audio-2.dll"
    #        "${SFML_ROOT}/bin/sfml-graphics-2.dll"
    #        "${SFML_ROOT}/bin/sfml-system-2.dll"
    #        "${SFML_ROOT}/bin/sfml-window-2.dll"
    #    DESTINATION .
    #    COMPONENT runtime
    #)

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

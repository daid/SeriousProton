project(SeriousProton)
cmake_minimum_required(VERSION 2.8.12)

option(ENABLE_CRASH_LOGGER "Enable the drmingw crash logging facilities" OFF)

if(DEFINED ENABLE_CRASH_LOGGER)
  if(WIN32)
    if(NOT DEFINED DRMINGW_ROOT)
      message(FATAL_ERROR "DRMINGW_ROOT was not set. Unable to continue")
    endif(NOT DEFINED DRMINGW_ROOT)
  endif(WIN32)
endif(DEFINED ENABLE_CRASH_LOGGER)

set(CPACK_PACKAGE_VERSION_MAJOR "2017")
set(CPACK_PACKAGE_VERSION_MINOR "01")
set(CPACK_PACKAGE_VERSION_PATCH "19")
set(SERIOUSPROTON_VERSION
    ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(APPLE)
  set(CMAKE_INSTALL_PREFIX "./")
endif()

if(NOT ${CMAKE_VERSION} VERSION_LESS 3.1)
  set(CMAKE_CXX_STANDARD 11)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(EXTRA_LIBS ${EXTRA_LIBS} Box2D)
set(EXTRA_LIBS ${EXTRA_LIBS} GLEW)
set(EXTRA_LIBS ${EXTRA_LIBS} sfml-audio)
set(EXTRA_LIBS ${EXTRA_LIBS} sfml-graphics)
set(EXTRA_LIBS ${EXTRA_LIBS} sfml-network)
set(EXTRA_LIBS ${EXTRA_LIBS} sfml-system)
set(EXTRA_LIBS ${EXTRA_LIBS} sfml-window)

set(SERIOUSPROTON_SRCS
  src/soundManager.cpp
  src/clipboard.cpp
  src/postProcessManager.cpp
  src/fixedSocket.cpp
  src/input.cpp
  src/tween.cpp
  src/multiplayer_client.cpp
  src/PlayerController.cpp
  src/logging.cpp
  src/random.cpp
  src/gameEntity.cpp
  src/multiplayer_server_scanner.cpp
  src/networkRecorder.cpp
  src/multiplayer_server.cpp
  src/scriptInterfaceMagic.cpp
  src/Renderable.cpp
  src/Updatable.cpp
  src/textureManager.cpp
  src/collisionable.cpp
  src/stringImproved.cpp
  src/resources.cpp
  src/scriptInterface.cpp
  src/event.cpp
  src/multiplayer.cpp
  src/httpServer.cpp
  src/networkAudioStream.cpp
  src/engine.cpp
  src/windowManager.cpp
  src/shaderManager.cpp
)

set(SERIOUSPROTON_HDRS
  src/clipboard.h
  src/collisionable.h
  src/engine.h
  src/event.h
  src/fixedSocket.h
  src/gameEntity.h
  src/httpServer.h
  src/input.h
  src/logging.h
  src/multiplayer_client.h
  src/multiplayer.h
  src/multiplayer_server.h
  src/multiplayer_server_scanner.h
  src/networkAudioStream.h
  src/networkRecorder.h
  src/P.h
  src/PlayerController.h
  src/postProcessManager.h
  src/random.h
  src/Renderable.h
  src/resources.h
  src/scriptInterface.h
  src/scriptInterfaceMagic.h
  src/shaderManager.h
  src/soundManager.h
  src/stringImproved.h
  src/textureManager.h
  src/tween.h
  src/Updatable.h
  src/vectorUtils.h
  src/windowManager.h
)

set(SERIOUSPROTON_DTOA_HDRS
    src/dtoa/dtoa_milo.h
)

set(SERIOUSPROTON_LUA_SRCS
  src/lua/lundump.c
  src/lua/lmem.c
  src/lua/lctype.c
  src/lua/lparser.c
  src/lua/lmathlib.c
  src/lua/lbaselib.c
  src/lua/ltm.c
  src/lua/lstrlib.c
  src/lua/lfunc.c
  src/lua/ldo.c
  src/lua/lcorolib.c
  src/lua/lcode.c
  src/lua/lapi.c
  src/lua/lstate.c
  src/lua/llex.c
  src/lua/lopcodes.c
  src/lua/lobject.c
  src/lua/ldebug.c
  src/lua/lstring.c
  src/lua/ltable.c
  src/lua/lzio.c
  src/lua/ldblib.c
  src/lua/ldump.c
  src/lua/lvm.c
  src/lua/loadlib.c
  src/lua/lauxlib.c
  src/lua/ltablib.c
  src/lua/lgc.c
  src/lua/lbitlib.c
  )

set(SERIOUSPROTON_LUA_HDRS
  src/lua/lapi.h
  src/lua/lauxlib.h
  src/lua/lcode.h
  src/lua/lctype.h
  src/lua/ldebug.h
  src/lua/ldo.h
  src/lua/lfunc.h
  src/lua/lgc.h
  src/lua/llex.h
  src/lua/llimits.h
  src/lua/lmem.h
  src/lua/lobject.h
  src/lua/lopcodes.h
  src/lua/lparser.h
  src/lua/lprefix.h
  src/lua/lstate.h
  src/lua/lstring.h
  src/lua/ltable.h
  src/lua/ltm.h
  src/lua/luaconf.h
  src/lua/lua.h
  src/lua/lua.hpp
  src/lua/lualib.h
  src/lua/lundump.h
  src/lua/lvm.h
  src/lua/lzio.h
  )

include_directories(src)
add_library(${PROJECT_NAME} SHARED
    ${SERIOUSPROTON_SRCS}
    ${SERIOUSPROTON_HDRS}
    ${SERIOUSPROTON_DTOA_HDRS}
    ${SERIOUSPROTON_LUA_SRCS}
    ${SERIOUSPROTON_LUA_HDRS}
)

target_link_libraries(${PROJECT_NAME} ${EXTRA_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME "SeriousProton"
  CLEAN_DIRECT_OUTPUT 1
  VERSION ${SERIOUSPROTON_VERSION}
)

install(FILES ${SERIOUSPROTON_HDRS} DESTINATION include/${PROJECT_NAME})
install(FILES ${SERIOUSPROTON_DTOA_HDRS} DESTINATION include/${PROJECT_NAME}/dtoa)
install(FILES ${SERIOUSPROTON_LUA_HDRS} DESTINATION include/${PROJECT_NAME}/lua)

install(TARGETS ${PROJECT_NAME} EXPORT SeriousProton-targets
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  ARCHIVE DESTINATION lib${LIB_SUFFIX})
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

install(EXPORT SeriousProton-targets DESTINATION lib${LIB_SUFFIX}/${PROJECT_NAME})
set (SERIOUSPROTON_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
set (SERIOUSPROTON_INCLUDE_DIRS ${SERIOUS_PROTON_INCLUDE_DIR} ${CMAKE_INSTALL_PREFIX}/include)
set (SERIOUSPROTON_LIBRARY_DIRS ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
set (SERIOUSPROTON_LIBRARY ${PROJECT_NAME})
set (SERIOUSPROTON_LIBRARIES ${SERIOUSPROTON_LIBRARY})
set (SERIOUSPROTON_USE_FILE
    ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/cmake/SeriousProton/UseSeriousProton.cmake)
configure_file(cmake/SeriousProtonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SeriousProtonConfig.cmake @ONLY ESCAPE_QUOTES)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SeriousProtonConfig.cmake cmake/UseSeriousProton.cmake
    DESTINATION lib${LIB_SUFFIX}/cmake/SeriousProton)

cmake_minimum_required(VERSION 3.7)
project(SeriousProton LANGUAGES CXX)
set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/
    ${CMAKE_MODULE_PATH}
)

#---------------------------------File lists-----------------------------------
set(source_files_relative #All SeriousProton's objects to compile
    src/clipboard.cpp
    src/collisionable.cpp
    src/engine.cpp
    src/event.cpp
    src/fixedSocket.cpp
    src/gameEntity.cpp
    src/httpServer.cpp
    src/input.cpp
    src/logging.cpp
    src/multiplayer.cpp
    src/multiplayer_client.cpp
    src/multiplayer_server.cpp
    src/multiplayer_server_scanner.cpp
    src/networkAudioStream.cpp
    src/networkRecorder.cpp
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
set(source_files "")
foreach(source_file_relative ${source_files_relative})
    list(APPEND source_files ${CMAKE_SOURCE_DIR}/${source_file_relative})
endforeach()

set(seriousproton_include_dir ${CMAKE_SOURCE_DIR}/src/)

#----------------------------------Compiling-----------------------------------
set(CMAKE_CXX_STANDARD 17)

add_library(seriousproton STATIC ${source_files})

target_include_directories(seriousproton
    PUBLIC    
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/libopus/include>
)
target_compile_definitions(seriousproton
    PUBLIC
        SFML_NO_DEPRECATED_WARNINGS # SFML 2.5 gives deprication warnings on a few functions we use. But we maintain 2.3 compatibility, so ignore those warnings.
        $<$<BOOL:${WIN32}>:NOMINMAX>
)

#--------------------------------Dependencies----------------------------------
include(ExternalProject)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/Box2D)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/lua)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/GL)

find_package(SFML 2.3 COMPONENTS system audio network window graphics)
if(NOT ${SFML_FOUND})
    message(STATUS "Couldn't find SFML. Building it from scratch. Installing to ${CMAKE_INSTALL_PREFIX}")
    ExternalProject_Add(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.4.2 #Always build with this tag, so we're sure it is stable.
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DBUILD_SHARED_LIBS=OFF
    )
    set(SFML_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include) #Otherwise this is filled by FindSFML.cmake.
    set(SFML_LIBRARIES
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sfml-audio${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sfml-graphics${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sfml-network${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sfml-system${CMAKE_STATIC_LIBRARY_SUFFIX}
        ${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sfml-window${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    add_dependencies(seriousproton SFML)
endif()

target_include_directories(seriousproton PUBLIC $<BUILD_INTERFACE:${SFML_INCLUDE_DIR}>)
target_link_libraries(seriousproton PUBLIC box2d lua glew ${SFML_LIBRARIES})

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

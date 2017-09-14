cmake_minimum_required(VERSION 3.5.1) #Only tested with 3.5.1 so far.
project(SeriousProton)
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
	list(APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/${source_file_relative})
endforeach()

set(header_files_relative #Header files that must be installed somewhere.
	src/clipboard.h
	src/collisionable.h
	src/engine.h
	src/event.h
	src/fixedSocket.h
	src/gameEntity.h
	src/httpServer.h
	src/input.h
	src/logging.h
	src/multiplayer.h
	src/multiplayer_client.h
	src/multiplayer_internal.h
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
set(header_files "")
foreach(header_file_relative ${header_files_relative})
	list(APPEND header_files ${CMAKE_CURRENT_SOURCE_DIR}/${header_file_relative})
endforeach()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

#----------------------------------Compiling-----------------------------------
set(CMAKE_CXX_STANDARD 11)
add_library(seriousproton STATIC ${source_files})

#--------------------------------Dependencies----------------------------------
include(ExternalProject)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/Box2D)
add_dependencies(seriousproton box2d)

find_package(SFML 2.4 COMPONENTS system audio network window graphics)
if(NOT ${SFML_FOUND})
	message(STATUS "Couldn't find SFML. Building it from scratch. Installing to ${CMAKE_INSTALL_PREFIX}")
	ExternalProject_Add(SFML
		GIT_REPOSITORY https://github.com/SFML/SFML.git
		GIT_TAG 2.4.2 #Always build with this tag, so we're sure it is stable.
		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	)
	set(SFML_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include) #Otherwise this is filled by FindSFML.cmake.
	set(SFML_LIBRARIES
		libsfml-audio
		libsfml-graphics
		libsfml-network
		libsfml-system
		libsfml-window
	)
endif()
include_directories(${SFML_INCLUDE_DIR})
target_link_libraries(seriousproton ${SFML_LIBRARIES})

#--------------------------------Installation----------------------------------
install(
	TARGETS seriousproton
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(
	FILES ${header_files}
	DESTINATION include
)

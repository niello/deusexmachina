cmake_minimum_required(VERSION 3.8.2) # source_group TREE
#cmake_policy(SET CMP0048 NEW)
project(deusexmachina) # VERSION 0.1.0)

# TODO: make project version work without breaking deps
# TODO: Detour's DebugDraw.h etc headers must be namespaced into a directory
# TODO: rename Detour's DebugUtils target!
# TODO: link only Detour, DetourCrowd & DebugUtils, don't link Recast & DetourTileCache if not used by main code
# TODO: try to use find_package for transparent handling of installed / source packages? Or always compile from source on any system?
# TODO: rewrite DEM_DISABLE_CXX_EXCEPTIONS in a target level instead of patching CMAKE_CXX_FLAGS
# TODO: use ARCHIVE_OUTPUT_DIRECTORY, LIBRARY_OUTPUT_DIRECTORY, RUNTIME_OUTPUT_DIRECTORY
# TODO: bullet variables must be with BULLET_ prefix even if we will build it separately
# TODO: option - reference dependencies by find_package and build separately or include as subdirs
# TODO: erase unnecessary options from third party CMake files. Deps submodule can come in two forms - one is completely untouched,
#       where user builds deps manually and then find_package finds them, and other is cleared of all unnecessary stuff, stored
#       in our special DEM-Deps (deusexmachina-dependencies) repo, added to DEM projects as subdirectories
# TODO: optional(?) exclude from all for third-party libs, script for rebuilding after dependency update
# TODO: disable RTTI & exceptions per target and in dependencies too!
# TODO: Configuration for debug DEM + release Deps
# TODO: lib output directories and names must be convenient (one dir, common debug prefix?)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#SET(CMAKE_SUPPRESS_REGENERATION  1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DEMRPG") # Not working for some reason...

option(DEM_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

include(CMake/DEMCMakeUtils.cmake)

if(MSVC AND DEM_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

#???run python script to update cmake src lists?

# Dependency: Lua

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/lua" "${CMAKE_BINARY_DIR}/Deps/lua")
set_target_properties(lua PROPERTIES FOLDER "Deps")

# Dependency: OGG & Theora

set(OGG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Deps/ogg")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/ogg" "${CMAKE_BINARY_DIR}/Deps/ogg")
set_target_properties(ogg PROPERTIES FOLDER "Deps")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/theora" "${CMAKE_BINARY_DIR}/Deps/theora")
set_target_properties(theora PROPERTIES FOLDER "Deps")

# Dependency: TinyXML2 (all unnecessary stuff is stripped out, so we don't use subdirectory's CMakeLists.txt)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/tinyxml2" "${CMAKE_BINARY_DIR}/Deps/tinyxml2")
set_target_properties(tinyxml2 PROPERTIES FOLDER "Deps")

# Dependency: CEGUI

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI/dependencies/lib")
	set(DEM_CEGUI_DEPS_FOUND TRUE)
else()
	find_package(PythonInterp)
	if(PYTHONINTERP_FOUND)
		execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/build_cegui_deps.py" "${CMAKE_GENERATOR}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE DEM_CEGUI_DEPS_BUILD_RESULT
		)
		if(${DEM_CEGUI_DEPS_BUILD_RESULT} EQUAL 0 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI/dependencies/lib")
			set(DEM_CEGUI_DEPS_FOUND TRUE)
		else()
			message(WARNING "Error occured when building CEGUI dependencies, CEGUI will not be used:
			${DEM_CEGUI_DEPS_BUILD_RESULT}")
		endif()
	else()
		message(WARNING "CEGUI dependencies are not built and no python found to run build script. You need to do it manually to use CEGUI.")
	endif()
endif()

if(DEM_CEGUI_DEPS_FOUND)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI" "${CMAKE_BINARY_DIR}/Deps/CEGUI")
	get_all_targets(DEM_CEGUI_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI")
	foreach(_TARGET_NAME IN LISTS DEM_CEGUI_TARGETS)
		set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps/CEGUI")
		if(MSVC AND DEM_DISABLE_CXX_EXCEPTIONS)
			target_compile_options(${_TARGET_NAME} PRIVATE "/wd4530") # Don't complain about exception unwinding
		endif()
	endforeach()
endif()

# Dependency: Bullet Physics

set(BUILD_CLSOCKET OFF CACHE INTERNAL "" FORCE)
set(BUILD_ENET OFF CACHE INTERNAL "" FORCE)

#find_package(Bullet)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/bullet" "${CMAKE_BINARY_DIR}/Deps/bullet")
get_all_targets(DEM_BULLET_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/bullet")
foreach(_TARGET_NAME IN LISTS DEM_BULLET_TARGETS)
	set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps/bullet")
endforeach()

# Dependency: Recast navigation libs

set(RECASTNAVIGATION_DEMO OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_STATIC ON CACHE INTERNAL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/recastnavigation" "${CMAKE_BINARY_DIR}/Deps/recastnavigation")
get_all_targets(DEM_RECASTNAV_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/recastnavigation")
foreach(_TARGET_NAME IN LISTS DEM_RECASTNAV_TARGETS)
	set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps/recastnavigation")
endforeach()

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Low/src" FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)
target_link_libraries(DEMLow PUBLIC lua theora tinyxml2)
if(DEM_CEGUI_TARGETS)
	target_include_directories(DEMLow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI/cegui/include" "${CMAKE_CURRENT_BINARY_DIR}/Deps/CEGUI/cegui/include")
	target_link_libraries(DEMLow PUBLIC ${DEM_CEGUI_TARGETS})
endif()
if(DEM_BULLET_TARGETS)
	target_include_directories(DEMLow PUBLIC "${BULLET_PHYSICS_SOURCE_DIR}/src")
	target_link_libraries(DEMLow PUBLIC ${DEM_BULLET_TARGETS})
endif()
if(DEM_RECASTNAV_TARGETS)
	target_link_libraries(DEMLow PUBLIC ${DEM_RECASTNAV_TARGETS})
endif()
if(MSVC AND DEM_DISABLE_CXX_RTTI)
	target_compile_options(DEMLow PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# DEMGame

include(CMake/DEMGame.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Game/src" FILES ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
add_library(DEMGame ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
target_include_directories(DEMGame PUBLIC DEM/Game/src)
target_link_libraries(DEMGame DEMLow)
if(MSVC AND DEM_DISABLE_CXX_RTTI)
	target_compile_options(DEMGame PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# DEMRPG

include(CMake/DEMRPG.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/RPG/src" FILES ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
add_library(DEMRPG ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
target_include_directories(DEMRPG PUBLIC DEM/RPG/src)
target_link_libraries(DEMRPG DEMGame DEMLow)
if(MSVC AND DEM_DISABLE_CXX_RTTI)
	target_compile_options(DEMRPG PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

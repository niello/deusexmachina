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

#SET(CMAKE_SUPPRESS_REGENERATION  1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DEMRPG")

option(DEM_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

include(CMake/DEMCMakeUtils.cmake)

if(MSVC AND DEM_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

#???run python script to update cmake src lists? check changed before touching

# Dependencies

#set(DEM_DEPS_DISABLE_CXX_EXCEPTIONS ${DEM_DISABLE_CXX_EXCEPTIONS} CACHE INTERNAL "" FORCE)
#set(DEM_DEPS_DISABLE_CXX_RTTI ${DEM_DISABLE_CXX_RTTI} CACHE INTERNAL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps" "${CMAKE_CURRENT_BINARY_DIR}/Deps")
get_all_targets(DEM_DEPS_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/Deps")
foreach(_TARGET_NAME IN LISTS DEM_DEPS_TARGETS)
	get_target_property(_TARGET_FOLDER ${_TARGET_NAME} FOLDER)
	if(_TARGET_FOLDER)
		set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps/${_TARGET_FOLDER}")
	else()
		set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps")
	endif()
endforeach()

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Low/src" FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)
target_link_libraries(DEMLow PUBLIC ${DEM_DEPS_TARGETS})
list(APPEND DEM_TARGETS DEMLow)

# DEMGame

include(CMake/DEMGame.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Game/src" FILES ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
add_library(DEMGame ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
target_include_directories(DEMGame PUBLIC DEM/Game/src)
target_link_libraries(DEMGame DEMLow)
list(APPEND DEM_TARGETS DEMGame)

# DEMRPG

include(CMake/DEMRPG.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/RPG/src" FILES ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
add_library(DEMRPG ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
target_include_directories(DEMRPG PUBLIC DEM/RPG/src)
target_link_libraries(DEMRPG DEMGame DEMLow)
list(APPEND DEM_TARGETS DEMRPG)

# Processing common settings for all dependency targets

# HACK: empty generator expressions are used for config suffix suppression
set(DEM_DEPS_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib/$<$<BOOL:FALSE>:>")
set(DEM_DEPS_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/$<$<BOOL:FALSE>:>")

foreach(_TARGET_NAME IN LISTS DEM_TARGETS)

	set_target_properties(${_TARGET_NAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${DEM_DEPS_LIB_DIR}
		LIBRARY_OUTPUT_DIRECTORY ${DEM_DEPS_LIB_DIR}
		RUNTIME_OUTPUT_DIRECTORY ${DEM_DEPS_BIN_DIR}
		DEBUG_POSTFIX "_d"
	)

	foreach(_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
		string(TOUPPER "${_CONFIG}" _CONFIG)
		set_target_properties(${_TARGET_NAME} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_LIB_DIR}
			RUNTIME_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_BIN_DIR}
		)
	endforeach()

	# Disable C++ RTTI if requested
	if(MSVC AND DEM_DISABLE_CXX_RTTI)
		target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
	endif()

endforeach()

cmake_minimum_required(VERSION 3.8.2) # source_group TREE
#cmake_policy(SET CMP0048 NEW)
project(DEM-Deps) # VERSION 0.1.0)

# TODO: make project version work without breaking deps
# TODO: Detour's DebugDraw.h etc headers must be namespaced into a directory
# TODO: rename Detour's DebugUtils target!
# TODO: link only Detour, DetourCrowd & DebugUtils, don't link Recast & DetourTileCache if not used by main code
# TODO: try to use find_package for transparent handling of installed / source packages? Or always compile from source on any system?
# TODO: rewrite DEM_DEPS_DISABLE_CXX_EXCEPTIONS in a target level instead of patching CMAKE_CXX_FLAGS
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
#CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD ON
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(DEM_DEPS_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DEPS_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

include(DEMCMakeUtils.cmake)

if(MSVC AND DEM_DEPS_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

#???run python script to update cmake src lists?

# Dependency: Lua

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lua" "${CMAKE_CURRENT_BINARY_DIR}/lua")
if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
	target_compile_options(lua PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# Dependency: OGG & Theora

set(OGG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ogg")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ogg" "${CMAKE_CURRENT_BINARY_DIR}/ogg")
if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
	target_compile_options(ogg PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/theora" "${CMAKE_CURRENT_BINARY_DIR}/theora")
if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
	target_compile_options(theora PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# Dependency: TinyXML2 (all unnecessary stuff is stripped out, so we don't use subdirectory's CMakeLists.txt)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2" "${CMAKE_CURRENT_BINARY_DIR}/tinyxml2")
if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
	target_compile_options(tinyxml2 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# Dependency: CEGUI

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/dependencies/lib")
	set(DEM_CEGUI_DEPS_FOUND TRUE)
else()
	find_package(PythonInterp)
	if(PYTHONINTERP_FOUND)
		execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/build_cegui_deps.py" "${CMAKE_GENERATOR}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE DEM_CEGUI_DEPS_BUILD_RESULT
		)
		if(${DEM_CEGUI_DEPS_BUILD_RESULT} EQUAL 0 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/dependencies/lib")
			set(DEM_CEGUI_DEPS_FOUND TRUE)
		else()
			message(WARNING "Error occured when building CEGUI dependencies, CEGUI will not be used:
			${DEM_CEGUI_DEPS_BUILD_RESULT}")
		endif()
	else()
		message(WARNING "CEGUI dependencies are not built and no python found to run build script. You need to do it manually to use CEGUI.")
	endif()
endif()

# NB: CEGUI ignores DEM_DEPS_DISABLE_CXX_RTTI because it uses dynamic_cast
if(DEM_CEGUI_DEPS_FOUND)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/CEGUI" "${CMAKE_CURRENT_BINARY_DIR}/CEGUI")
	get_all_targets(DEM_CEGUI_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI")
	foreach(_TARGET_NAME IN LISTS DEM_CEGUI_TARGETS)
		set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "CEGUI")
		target_include_directories(${_TARGET_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/cegui/include" "${CMAKE_CURRENT_BINARY_DIR}/CEGUI/cegui/include")
		if(MSVC AND DEM_DEPS_DISABLE_CXX_EXCEPTIONS)
			target_compile_options(${_TARGET_NAME} PRIVATE "/wd4530") # Don't complain about exception unwinding
		endif()
	endforeach()
endif()

# Dependency: Bullet Physics

set(BUILD_CLSOCKET OFF CACHE INTERNAL "" FORCE)
set(BUILD_ENET OFF CACHE INTERNAL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/bullet" "${CMAKE_CURRENT_BINARY_DIR}/bullet")
get_all_targets(DEM_BULLET_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/bullet")
foreach(_TARGET_NAME IN LISTS DEM_BULLET_TARGETS)
	set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "bullet")
	target_include_directories(${_TARGET_NAME} INTERFACE "${BULLET_PHYSICS_SOURCE_DIR}/src")
	if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
		target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
	endif()
endforeach()

# Dependency: Recast navigation libs

set(RECASTNAVIGATION_DEMO OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_STATIC ON CACHE INTERNAL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation" "${CMAKE_CURRENT_BINARY_DIR}/recastnavigation")
get_all_targets(DEM_RECASTNAV_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation")
foreach(_TARGET_NAME IN LISTS DEM_RECASTNAV_TARGETS)
	set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "recastnavigation")
	if(MSVC AND DEM_DEPS_DISABLE_CXX_RTTI)
		target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
	endif()
endforeach()

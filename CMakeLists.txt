cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(deusexmachina)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DEMRPG") # Not working for some reason...

option(DEM_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

if(MSVC AND DEM_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

function(get_all_targets _result _dir)
	get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
	foreach(_subdir IN LISTS _subdirs)
		get_all_targets(${_result} "${_subdir}")
	endforeach()
	get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
	set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

#???run python script to update cmake src lists?

# Dependency: Lua

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/lua" "${CMAKE_BINARY_DIR}/Deps/lua")
set_target_properties(lua PROPERTIES FOLDER "Deps")

# Dependency: TinyXML2 (all unnecessary stuff is stripped out, so we don't use subdirectory's CMakeLists.txt)

#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2" "${CMAKE_BINARY_DIR}/tinyxml2")
add_library(tinyxml2 "${CMAKE_CURRENT_SOURCE_DIR}/Deps/tinyxml2/tinyxml2.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Deps/tinyxml2/tinyxml2.h")
set_target_properties(tinyxml2 PROPERTIES FOLDER "Deps" DEBUG_POSTFIX "_d")
target_include_directories(tinyxml2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Deps/tinyxml2")
target_compile_definitions(tinyxml2 PUBLIC TINYXML2_EXPORT $<$<CONFIG:Debug>:TINYXML2_DEBUG>) #???need TINYXML2_EXPORT for static lib?
if(MSVC)
	target_compile_definitions(tinyxml2 PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

# Dependency: CEGUI

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI/dependencies/lib")
	set(DEM_CEGUI_DEPS_FOUND TRUE)
else()
	find_package(PythonInterp)
	if(PYTHONINTERP_FOUND)
		execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/build_cegui_deps.py" "${CMAKE_GENERATOR}"
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
	foreach(TARGET_NAME IN LISTS DEM_CEGUI_TARGETS)
		set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Deps/CEGUI")
	endforeach()
endif()

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Low/src" FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)
target_link_libraries(DEMLow lua tinyxml2)
if(DEM_CEGUI_TARGETS)
	target_include_directories(DEMLow PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Deps/CEGUI/cegui/include" "${CMAKE_CURRENT_BINARY_DIR}/Deps/CEGUI/cegui/include")
	target_link_libraries(DEMLow ${DEM_CEGUI_TARGETS})
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

cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(deusexmachina)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DEMRPG") # Not working for some reason...

option(DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

if(MSVC AND DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

# Deps

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps/lua" "${CMAKE_BINARY_DIR}/Deps/lua")
set_target_properties(lua PROPERTIES FOLDER "Deps")

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE DEM/Low/src FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)
target_link_libraries(DEMLow lua)
if(MSVC AND DISABLE_CXX_RTTI)
	target_compile_options(DEMLow PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# DEMGame

include(CMake/DEMGame.cmake)
source_group(TREE DEM/Game/src FILES ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
add_library(DEMGame ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
target_include_directories(DEMGame PUBLIC DEM/Game/src)
target_link_libraries(DEMGame DEMLow)
if(MSVC AND DISABLE_CXX_RTTI)
	target_compile_options(DEMGame PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

# DEMRPG

include(CMake/DEMRPG.cmake)
source_group(TREE DEM/RPG/src FILES ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
add_library(DEMRPG ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
target_include_directories(DEMRPG PUBLIC DEM/RPG/src)
target_link_libraries(DEMRPG DEMGame DEMLow)
if(MSVC AND DISABLE_CXX_RTTI)
	target_compile_options(DEMRPG PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
endif()

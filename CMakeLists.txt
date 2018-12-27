cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(deusexmachina)

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE DEM/Low/src FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)

# DEMGame

include(CMake/DEMGame.cmake)
source_group(TREE DEM/Game/src FILES ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
add_library(DEMGame ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
target_include_directories(DEMGame PUBLIC DEM/Game/src)
target_link_libraries(DEMGame DEMLow)

# DEMRPG

include(CMake/DEMRPG.cmake)
source_group(TREE DEM/RPG/src FILES ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
add_library(DEMRPG ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
target_include_directories(DEMRPG PUBLIC DEM/RPG/src)
target_link_libraries(DEMRPG DEMGame DEMLow)

cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-effect)

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_EFFECT_HEADERS} ${DEM_CF_EFFECT_SOURCES})
add_executable(cf-effect ${DEM_CF_EFFECT_HEADERS} ${DEM_CF_EFFECT_SOURCES})
target_include_directories(cf-effect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-effect PRIVATE ToolsCommon)
set_target_properties(cf-effect PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

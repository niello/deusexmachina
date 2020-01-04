cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-skybox)

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_SKY_HEADERS} ${DEM_CF_SKY_SOURCES})
add_executable(cf-skybox ${DEM_CF_SKY_HEADERS} ${DEM_CF_SKY_SOURCES})
target_include_directories(cf-skybox PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-skybox PRIVATE ToolsCommon)
set_target_properties(cf-skybox PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

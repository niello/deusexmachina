cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-material)

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_MTL_HEADERS} ${DEM_CF_MTL_SOURCES})
add_executable(cf-material ${DEM_CF_MTL_HEADERS} ${DEM_CF_MTL_SOURCES})
target_include_directories(cf-material PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-material PRIVATE ToolsCommon)
set_target_properties(cf-material PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

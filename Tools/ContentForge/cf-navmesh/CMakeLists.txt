cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-navmesh)

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_NAV_HEADERS} ${DEM_CF_NAV_SOURCES})
add_executable(cf-navmesh ${DEM_CF_NAV_HEADERS} ${DEM_CF_NAV_SOURCES})
target_include_directories(cf-navmesh PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-navmesh PRIVATE ToolsCommon)
set_target_properties(cf-navmesh PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

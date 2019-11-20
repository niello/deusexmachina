cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-rpath)

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_RPATH_HEADERS} ${DEM_CF_RPATH_SOURCES})
add_executable(cf-rpath ${DEM_CF_RPATH_HEADERS} ${DEM_CF_RPATH_SOURCES})
target_include_directories(cf-rpath PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-rpath PRIVATE ToolsCommon)
set_target_properties(cf-rpath PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

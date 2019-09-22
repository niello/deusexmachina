cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-Common)

include(src.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_TOOLS_COMMON_HEADERS} ${DEM_TOOLS_COMMON_SOURCES})
add_library(ToolsCommon STATIC ${DEM_TOOLS_COMMON_HEADERS} ${DEM_TOOLS_COMMON_SOURCES})
target_include_directories(ToolsCommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CLI11")
set_target_properties(ToolsCommon PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON) # At least for std::filesystem

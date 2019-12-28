cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-l3dt)

# Dependency: pugixml
set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Deps/pugixml" "${CMAKE_CURRENT_BINARY_DIR}/Deps/pugixml")
set_target_properties(pugixml PROPERTIES FOLDER "Deps")

# Tool

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_L3DT_HEADERS} ${DEM_CF_L3DT_SOURCES})
add_executable(cf-l3dt ${DEM_CF_L3DT_HEADERS} ${DEM_CF_L3DT_SOURCES})
target_include_directories(cf-l3dt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cf-l3dt PRIVATE ToolsCommon pugixml)
set_target_properties(cf-l3dt PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

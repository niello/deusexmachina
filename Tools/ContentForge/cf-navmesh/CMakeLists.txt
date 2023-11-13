cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-navmesh)

# Dependency: Recast+Detour navigation mesh building library
set(RECASTNAVIGATION_DEMO OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_STATIC ON CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Deps/recastnavigation/Recast" "${CMAKE_CURRENT_BINARY_DIR}/recastnavigation/Recast")
set_target_properties(Recast PROPERTIES FOLDER "Deps")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Deps/recastnavigation/Detour" "${CMAKE_CURRENT_BINARY_DIR}/recastnavigation/Detour")
set_target_properties(Detour PROPERTIES FOLDER "Deps")

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_NAV_HEADERS} ${DEM_CF_NAV_SOURCES})
add_executable(cf-navmesh ${DEM_CF_NAV_HEADERS} ${DEM_CF_NAV_SOURCES})
target_include_directories(cf-navmesh PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../Deps/rtm/includes")
target_link_libraries(cf-navmesh PRIVATE ToolsCommon Recast Detour)
set_target_properties(cf-navmesh PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

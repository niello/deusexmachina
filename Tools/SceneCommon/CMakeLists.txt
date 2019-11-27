cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-SceneCommon)

include(src.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_SCENE_COMMON_HEADERS} ${DEM_SCENE_COMMON_SOURCES})
add_library(SceneCommon STATIC ${DEM_SCENE_COMMON_HEADERS} ${DEM_SCENE_COMMON_SOURCES})
target_include_directories(SceneCommon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/acl/includes")
target_link_libraries(SceneCommon PUBLIC ToolsCommon)
target_link_libraries(SceneCommon PRIVATE meshoptimizer)
set_target_properties(SceneCommon PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON) # At least for std::filesystem

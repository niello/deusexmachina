cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-ContentForge)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Tool: cf-hlsl

# TODO: get rid of ch-hlsl folder inside VS!

include("${CMAKE_CURRENT_SOURCE_DIR}/cf-hlsl/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_HLSL_HEADERS} ${DEM_CF_HLSL_SOURCES})
add_executable(cf-hlsl ${DEM_CF_HLSL_HEADERS} ${DEM_CF_HLSL_SOURCES})
target_include_directories(cf-hlsl PRIVATE src)
target_link_libraries(cf-hlsl PRIVATE sqlite3 ToolsCommon ShaderCompiler) # ShaderCompiler
set_target_properties(cf-hlsl PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON) # At least for std::filesystem

cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-ContentForge)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Dependency: meshoptimizer (cf-fbx, cf-gltf)
set(BUILD_DEMO OFF CACHE INTERNAL "" FORCE)
set(BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/meshoptimizer" "${CMAKE_CURRENT_BINARY_DIR}/Deps/meshoptimizer")

# Tools
add_subdirectory(cf-hlsl)
add_subdirectory(cf-effect)
add_subdirectory(cf-rpath)
add_subdirectory(cf-material)
add_subdirectory(cf-fbx)
add_subdirectory(cf-gltf)

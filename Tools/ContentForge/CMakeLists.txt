cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-ContentForge)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Tools
add_subdirectory(cf-hlsl)
add_subdirectory(cf-effect)
add_subdirectory(cf-rpath)
add_subdirectory(cf-material)
add_subdirectory(cf-fbx)
add_subdirectory(cf-gltf)
add_subdirectory(cf-l3dt)
add_subdirectory(cf-skybox)
add_subdirectory(cf-navmesh)

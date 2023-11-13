cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools-SceneCommon)

# Dependency: meshoptimizer
set(BUILD_DEMO OFF CACHE INTERNAL "" FORCE)
set(BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/meshoptimizer" "${CMAKE_CURRENT_BINARY_DIR}/Deps/meshoptimizer")
set_target_properties(meshoptimizer PROPERTIES FOLDER "Deps")

# Dependency: DevIL
set(IL_TESTS OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
# For now use libpng and libjpeg from CEGUI
set(IL_PNG_LIB ON CACHE BOOL "" FORCE) 
set(PNG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/include")
set(IL_JPEG_LIB ON CACHE BOOL "" FORCE)
set(JPEG_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/include")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}") # DevIL CMake BUG - Jasper tries to find JPEG package in a regular way and fails, breaking generation
#set(PNG_LIBRARIES ...) # See target_link_libraries below
#set(JPEG_LIBRARIES ...) # See target_link_libraries below
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/DevIL/DevIL/src-IL" "${CMAKE_CURRENT_BINARY_DIR}/Deps/DevIL/IL")
set_target_properties(IL PROPERTIES FOLDER "Deps")
target_compile_definitions(IL PUBLIC IL_STATIC_LIB)

# Dependency: Bullet Physics' LinearMath (for convex hull collision shape generation from mesh)
set(BULLET_PHYSICS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/bullet" CACHE INTERNAL "" FORCE)
FILE(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/bullet/VERSION" BULLET_VERSION)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/bullet/src/LinearMath" "${CMAKE_CURRENT_BINARY_DIR}/Deps/bullet/LinearMath")

# SceneCommon
include(src.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_SCENE_COMMON_HEADERS} ${DEM_SCENE_COMMON_SOURCES})
add_library(SceneCommon STATIC ${DEM_SCENE_COMMON_HEADERS} ${DEM_SCENE_COMMON_SOURCES})
target_include_directories(SceneCommon PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/rtm/includes"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/acl/includes"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/DevIL/DevIL/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/bullet/src"
)
target_link_libraries(SceneCommon PUBLIC ToolsCommon)
target_link_libraries(SceneCommon PRIVATE meshoptimizer IL LinearMath)
# Instead of setting PNG_LIBRARIES and JPEG_LIBRARIES for DevIL, link here to handle debug/release versions
target_link_libraries(SceneCommon PRIVATE
	debug "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/libpng_d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/libpng.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/jpeg_d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/jpeg.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/zlib_d.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/CEGUI/dependencies/lib/static/${DEM_TOOLS_ARCH_SUFFIX}/zlib.lib"
)
set_target_properties(SceneCommon PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON) # At least for std::filesystem

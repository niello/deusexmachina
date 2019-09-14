cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(ShaderCompiler)

# We don't disable exceptions & RTTI in tools, because they aren't runtime.
# If required, sample setup can be found in engine dependencies CMakeLists.txt.

# Dependency: SQLite 3
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Deps/sqlite" "${CMAKE_CURRENT_BINARY_DIR}/Deps/sqlite")

include(src.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${DEM_SHADER_COMPILER_HEADERS} ${DEM_SHADER_COMPILER_SOURCES})
add_library(ShaderCompiler SHARED ${DEM_SHADER_COMPILER_HEADERS} ${DEM_SHADER_COMPILER_SOURCES})
target_include_directories(ShaderCompiler PUBLIC src)
target_link_libraries(ShaderCompiler PUBLIC sqlite3)
if(WIN32)
	target_compile_definitions(ShaderCompiler PUBLIC DEM_PLATFORM_WIN32=1) # Public because app uses headers. Exclude headers when not Win32!
	target_link_libraries(ShaderCompiler PUBLIC DXGI.lib D3D11.lib) # TODO: only when D3D11 renderer is built
	target_link_libraries(ShaderCompiler PUBLIC D3d9.lib dxguid.lib) # TODO: only when D3D9 renderer is built
endif()

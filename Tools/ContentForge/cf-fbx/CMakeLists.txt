cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-fbx)

# Setup default path to make user's life easier. Another version of SDK can still be specified.
set(FBX_SDK_PATH "$ENV{PROGRAMFILES}/Autodesk/FBX/FBX SDK/2020.0/" CACHE PATH "FBX SDK path")

if(${MSVC_TOOLSET_VERSION} EQUAL 142)
  set(FBX_SDK_COMPILER_SUBFOLDER vs2019)
elseif(${MSVC_TOOLSET_VERSION} EQUAL 141)
  set(FBX_SDK_COMPILER_SUBFOLDER vs2017)
elseif(${MSVC_TOOLSET_VERSION} EQUAL 140)
  set(FBX_SDK_COMPILER_SUBFOLDER vs2015)
else()
  message(FATAL_ERROR "Unsupported Visual Studio version. Even if FBX SDK exists, your VS doesn't support C++17")
endif()

set(FBX_SDK_LIB_PATH_DEBUG "${FBX_SDK_PATH}/lib/${FBX_SDK_COMPILER_SUBFOLDER}/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/debug/")
set(FBX_SDK_LIB_PATH_RELEASE "${FBX_SDK_PATH}/lib/${FBX_SDK_COMPILER_SUBFOLDER}/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/release/")

include("${CMAKE_CURRENT_SOURCE_DIR}/src.cmake")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DEM_CF_FBX_HEADERS} ${DEM_CF_FBX_SOURCES})
add_executable(cf-fbx ${DEM_CF_FBX_HEADERS} ${DEM_CF_FBX_SOURCES})
target_include_directories(cf-fbx PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${FBX_SDK_PATH}/include")
target_link_libraries(cf-fbx PRIVATE
	SceneCommon
	debug ${FBX_SDK_LIB_PATH_DEBUG}libfbxsdk-md.lib optimized ${FBX_SDK_LIB_PATH_RELEASE}libfbxsdk-md.lib
	debug ${FBX_SDK_LIB_PATH_DEBUG}libxml2-md.lib optimized ${FBX_SDK_LIB_PATH_RELEASE}libxml2-md.lib
	debug ${FBX_SDK_LIB_PATH_DEBUG}zlib-md.lib optimized ${FBX_SDK_LIB_PATH_RELEASE}zlib-md.lib)
set_target_properties(cf-fbx PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(cf-fbx)

# Setup default FBX SDK path to make user's life easier. Another version of SDK can still be specified.
set(DEFAULT_FBX_SDK_PATH "")
if(NOT DEFINED CACHE{FBX_SDK_PATH} OR "${FBX_SDK_PATH}" STREQUAL "" OR NOT EXISTS "${FBX_SDK_PATH}/include")
	unset(FBX_SDK_PATH CACHE)
	set(FBX_ROOT "$ENV{PROGRAMFILES}/Autodesk/FBX/FBX SDK/")
	set(FBX_BEST_VERSION "0.0.0")
	if(EXISTS ${FBX_ROOT})
		file(GLOB FBX_SUBDIRS RELATIVE ${FBX_ROOT} "${FBX_ROOT}/*")
		foreach(FBX_SUBDIR ${FBX_SUBDIRS})
			if(IS_DIRECTORY ${FBX_ROOT}/${FBX_SUBDIR})
				if(${CMAKE_SIZEOF_VOID_P} LESS 8 AND ${FBX_SUBDIR} VERSION_GREATER_EQUAL "2020.3.4")
					message("cf-fbx: FBX SDK ${FBX_SUBDIR} skipped because it doesn't support 32 bit x86")
				else()
					message("cf-fbx: FBX SDK ${FBX_SUBDIR} found")
					if(${FBX_SUBDIR} VERSION_GREATER_EQUAL ${FBX_BEST_VERSION})
						set(FBX_BEST_VERSION ${FBX_SUBDIR})
					endif()
				endif()
			endif()
		endforeach()
	endif()
	if (${FBX_BEST_VERSION} VERSION_GREATER_EQUAL "0.0.0")
		message("cf-fbx: FBX SDK ${FBX_BEST_VERSION} selected")
		set(DEFAULT_FBX_SDK_PATH "${FBX_ROOT}/${FBX_BEST_VERSION}")
	else()
		message(WARNING "cf-fbx: FBX SDK installation not found")
	endif()
else()
	message("cf-fbx: FBX SDK path is ${FBX_SDK_PATH}")
endif()

set(FBX_SDK_PATH "${DEFAULT_FBX_SDK_PATH}" CACHE PATH "FBX SDK path")

# All MSVC 14X libraries are binary compatible
if(${MSVC_TOOLSET_VERSION} GREATER_EQUAL 143 AND EXISTS "${FBX_SDK_PATH}/lib/vs2022")
	set(FBX_SDK_COMPILER_SUBFOLDER vs2022)
elseif(${MSVC_TOOLSET_VERSION} GREATER_EQUAL 142 AND EXISTS "${FBX_SDK_PATH}/lib/vs2019")
	set(FBX_SDK_COMPILER_SUBFOLDER vs2019)
elseif(${MSVC_TOOLSET_VERSION} GREATER_EQUAL 141 AND EXISTS "${FBX_SDK_PATH}/lib/vs2017")
	set(FBX_SDK_COMPILER_SUBFOLDER vs2017)
elseif(${MSVC_TOOLSET_VERSION} GREATER_EQUAL 140)
	set(FBX_SDK_COMPILER_SUBFOLDER vs2015)
else()
	message(FATAL_ERROR "cf-fbx: Unsupported Visual Studio version. Even if FBX SDK exists, your VS doesn't support C++17")
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

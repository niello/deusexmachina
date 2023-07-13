cmake_minimum_required(VERSION 3.8.2) # source_group TREE
#cmake_policy(SET CMP0048 NEW)
project(deusexmachina) # VERSION 0.1.0)

# TODO: make project version work without breaking deps (CMP0048)
# TODO: exclude platform-specific sources & headers from unsupported platforms,?hide DEM_PLATFORM_WIN32 etc to PRIVATE?
# TODO: conditional addition of Win32, D3D9, D3D11 sources, headers & libs

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DEMRPG")

option(DEM_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)
set(DEM_PREBUILT_DEPS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Deps/Build" CACHE STRING "Prebuilt dependency package location")
if(EXISTS ${DEM_PREBUILT_DEPS_PATH})
	option(DEM_PREBUILT_DEPS "Use prebuilt dependencies" ON)
else()
	option(DEM_PREBUILT_DEPS "Use prebuilt dependencies" OFF)
endif()

include(CMake/DEMCMakeUtils.cmake)

if(MSVC AND DEM_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

# Dependencies

if(DEM_PREBUILT_DEPS)
	# Prebuild dependency package can be built from source once for necessary configuration and
	# its targets will not appear in a build system. It is recommended for game app development.
	#set(DEMDeps_ROOT ${DEM_PREBUILT_DEPS_PATH})
	set(DEMDeps_DIR "${DEM_PREBUILT_DEPS_PATH}" CACHE INTERNAL "" FORCE) # Suppress GUI entry without DEM_ prefix as it is not grouped to DEM
	find_package(DEMDeps)
	set(DEM_DEPS_POSSIBLE_TARGETS DEMDeps::lua DEMDeps::ogg DEMDeps::theora DEMDeps::CEGUIBase-0_Static DEMDeps::CEGUIPugiXMLParser_Static DEMDeps::CEGUISILLYImageCodec_Static DEMDeps::CEGUICoreWindowRendererSet_Static DEMDeps::CEGUICommonDialogs-0_Static DEMDeps::LinearMath DEMDeps::BulletCollision DEMDeps::BulletDynamics DEMDeps::DebugUtils DEMDeps::Detour DEMDeps::DetourCrowd DEMDeps::DetourTileCache DEMDeps::Recast)
	foreach(_TARGET_NAME IN LISTS DEM_DEPS_POSSIBLE_TARGETS)
		if(TARGET ${_TARGET_NAME})
			list(APPEND DEM_DEPS_TARGETS ${_TARGET_NAME})
		endif()
	endforeach()
else()
	# Dependencies are built from sources as a part of DEM engine with the same configuration and
	# appear in a build system. It is recommended for engine development and 3rd-party debugging.

	#set(DEM_DEPS_DISABLE_CXX_EXCEPTIONS ${DEM_DISABLE_CXX_EXCEPTIONS} CACHE INTERNAL "" FORCE)
	#set(DEM_DEPS_DISABLE_CXX_RTTI ${DEM_DISABLE_CXX_RTTI} CACHE INTERNAL "" FORCE)

	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Deps" "${CMAKE_CURRENT_BINARY_DIR}/Deps")
	get_all_targets(DEM_DEPS_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/Deps")
	foreach(_TARGET_NAME IN LISTS DEM_DEPS_TARGETS)
		get_target_property(_TARGET_TYPE ${_TARGET_NAME} TYPE)
		if (NOT ${_TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
			get_target_property(_TARGET_FOLDER ${_TARGET_NAME} FOLDER)
			if(_TARGET_FOLDER)
				set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps/${_TARGET_FOLDER}")
			else()
				set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "Deps")
			endif()
		endif()
	endforeach()

	#???set aliases with namespace DEMDeps:: for all targets? or use namespaces DEM_DEPS_TARGETS in prebuilt case instead?
endif()

# DEMLow

include(CMake/DEMLow.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Low/src" FILES ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
add_library(DEMLow ${DEM_L1_LOW_HEADERS} ${DEM_L1_LOW_SOURCES})
target_include_directories(DEMLow PUBLIC DEM/Low/src)
target_link_libraries(DEMLow PUBLIC ${DEM_DEPS_TARGETS})
if(WIN32)
	target_compile_definitions(DEMLow PUBLIC DEM_PLATFORM_WIN32=1) # Public because app uses headers. Exclude headers when not Win32!
	target_link_libraries(DEMLow PUBLIC UxTheme.lib Secur32.lib DbgHelp.lib) # Win32 platform, window and stack trace
	target_link_libraries(DEMLow PUBLIC DXGI.lib D3D11.lib) # TODO: only when D3D11 renderer is built
	target_link_libraries(DEMLow PUBLIC D3d9.lib dxguid.lib) # TODO: only when D3D9 renderer is built
	#TODO: move DEM_RENDER_DEBUG to cmake, link D3D11d.lib, use CMake target_link_libraries with debug/optimized/general
endif()
if(DEM_DISABLE_CXX_EXCEPTIONS)
	target_compile_definitions(DEMLow PUBLIC _HAS_EXCEPTIONS=0)
endif()
list(APPEND DEM_TARGETS DEMLow)

# DEMGame

include(CMake/DEMGame.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/Game/src" FILES ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
add_library(DEMGame ${DEM_L2_GAME_HEADERS} ${DEM_L2_GAME_SOURCES})
target_include_directories(DEMGame PUBLIC DEM/Game/src)
target_link_libraries(DEMGame DEMLow)
list(APPEND DEM_TARGETS DEMGame)

# DEMRPG

include(CMake/DEMRPG.cmake)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/DEM/RPG/src" FILES ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
add_library(DEMRPG ${DEM_L3_RPG_HEADERS} ${DEM_L3_RPG_SOURCES})
target_include_directories(DEMRPG PUBLIC DEM/RPG/src)
target_link_libraries(DEMRPG DEMGame DEMLow)
list(APPEND DEM_TARGETS DEMRPG)

# Processing common settings for all DEM targets

# HACK: empty generator expressions are used for config suffix suppression
set(DEM_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib/$<$<BOOL:FALSE>:>")
set(DEM_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/$<$<BOOL:FALSE>:>")

if(MSVC)
	foreach(_TARGET_NAME IN LISTS DEM_DEPS_TARGETS)
		get_target_property(type ${_TARGET_NAME} TYPE)
		if (NOT ${type} STREQUAL "INTERFACE_LIBRARY")
			set_target_properties(${_TARGET_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
		endif()
	endforeach()
endif()

foreach(_TARGET_NAME IN LISTS DEM_TARGETS)

	set_target_properties(${_TARGET_NAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${DEM_LIB_DIR}
		LIBRARY_OUTPUT_DIRECTORY ${DEM_LIB_DIR}
		RUNTIME_OUTPUT_DIRECTORY ${DEM_BIN_DIR}
		DEBUG_POSTFIX "_d"
		CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
	)

	foreach(_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
		string(TOUPPER "${_CONFIG}" _CONFIG)
		set_target_properties(${_TARGET_NAME} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_LIB_DIR}
			RUNTIME_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_BIN_DIR}
		)
	endforeach()

	if(MSVC)
		set_target_properties(${_TARGET_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
		if(DEM_DISABLE_CXX_RTTI)
			target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
		endif()
		target_compile_options(${_TARGET_NAME} PRIVATE "/Zc:__cplusplus")
	endif()

endforeach()

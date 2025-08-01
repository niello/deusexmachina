cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Deps)

# TODO: Detour's DebugDraw.h etc headers must be namespaced into a directory
# TODO: bullet variables must be with BULLET_ prefix even if we will build it separately
# TODO: set & hide from GUI unnecessary options of third-party CMake projects

# Directory structure is copied verbatim. Relative dest is relative to CMAKE_INSTALL_PREFIX.
#install(DIRECTORY src/ DESTINATION include/myproj FILES_MATCHING PATTERN "*.h")
#install(TARGETS ... EXPORT <export_name> ...)
#install(EXPORT <export_name> ...)
# The install() command generates a file, cmake_install.cmake, inside the build directory
# cmake -DBUILD_TYPE=Debug -P cmake_install.cmake
# cmake --build . --target install

#CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD ON
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(DEM_DEPS_DISABLE_CXX_EXCEPTIONS "Disable C++ exceptions" ON)
option(DEM_DEPS_DISABLE_CXX_RTTI "Disable C++ RTTI" ON)

include(DEMCMakeUtils.cmake)

if(MSVC AND DEM_DEPS_DISABLE_CXX_EXCEPTIONS)
	if(CMAKE_CXX_FLAGS MATCHES "/EHsc")
		string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
endif()

#???run python script to update cmake src lists?

# Dependency: {fmt}
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/fmt") # target: fmt or alias fmt::fmt
target_compile_definitions(fmt PRIVATE FMT_USE_EXCEPTIONS=0)

# Dependency: RTM

add_library(rtm INTERFACE)
target_include_directories(rtm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/rtm/includes")

# Dependency: ACL

add_library(acl INTERFACE)
target_include_directories(acl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/acl/includes")

# ACL & RTM custom targets for displaying files in IDE

include("${CMAKE_CURRENT_SOURCE_DIR}/acl/cmake/CMakeUtils.cmake")
file(GLOB_RECURSE ACL_INCLUDE_FILES LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/acl/includes/*.h)
create_source_groups("${ACL_INCLUDE_FILES}" ${CMAKE_CURRENT_SOURCE_DIR}/acl)
file(GLOB_RECURSE NATVIS_FILES LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/acl/*.natvis)
add_custom_target(acl_ SOURCES ${ACL_INCLUDE_FILES} ${NATVIS_FILES})
file(GLOB_RECURSE RTM_INCLUDE_FILES LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/rtm/includes/*.h)
create_source_groups("${RTM_INCLUDE_FILES}" ${CMAKE_CURRENT_SOURCE_DIR}/rtm)
add_custom_target(rtm_ SOURCES ${RTM_INCLUDE_FILES})

# Dependency: Lua

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lua" "${CMAKE_CURRENT_BINARY_DIR}/lua")

# Dependency: Sol 3

add_library(sol INTERFACE)
target_include_directories(sol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sol")
target_compile_definitions(sol INTERFACE SOL_ALL_SAFETIES_ON) # SOL_NO_EXCEPTIONS)

# Dependency: Tracy profiler

#set(TRACY_ENABLE $<IF:$<CONFIG:RelWithDebInfo>,ON,OFF>) - generator expression is useless at configure step
option(TRACY_ENABLE "" ON)
option(TRACY_ON_DEMAND "" ON)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tracy") # target: TracyClient or alias Tracy::TracyClient

# Dependency: OGG & Theora

set(OGG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ogg")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ogg" "${CMAKE_CURRENT_BINARY_DIR}/ogg")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/theora" "${CMAKE_CURRENT_BINARY_DIR}/theora")

# Dependency: CEGUI

set(CEGUI_BUILD_STATIC_CONFIGURATION ON CACHE INTERNAL "" FORCE)
set(CEGUI_BUILD_DYNAMIC_CONFIGURATION OFF CACHE INTERNAL "" FORCE)
set(CEGUI_BUILD_STATIC_FACTORY_MODULE ON CACHE INTERNAL "" FORCE)
set(CEGUI_BUILD_RENDERER_DIRECT3D11 OFF CACHE INTERNAL "" FORCE)
set(CEGUI_BUILD_PYTHON_MODULES_SWIG OFF CACHE INTERNAL "" FORCE)
set(CEGUI_BUILD_COMMON_DIALOGS OFF CACHE INTERNAL "" FORCE)
set(CEGUI_USE_RAQM OFF CACHE INTERNAL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/dependencies/lib")
	set(DEM_CEGUI_DEPS_FOUND TRUE)
else()
	find_package(PythonInterp)
	if(PYTHONINTERP_FOUND)
		execute_process(COMMAND "pip" "install" "-r" "${CMAKE_CURRENT_SOURCE_DIR}/../CMake/requirements.txt")
		execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/build_cegui_deps.py" "${CMAKE_GENERATOR}" "${CMAKE_GENERATOR_PLATFORM}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			RESULT_VARIABLE DEM_CEGUI_DEPS_BUILD_RESULT
		)
		if(${DEM_CEGUI_DEPS_BUILD_RESULT} EQUAL 0 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/dependencies/lib")
			set(DEM_CEGUI_DEPS_FOUND TRUE)
		else()
			message(WARNING "Error occured when building CEGUI dependencies, CEGUI will not be used:
			${DEM_CEGUI_DEPS_BUILD_RESULT}")
		endif()
	else()
		message(WARNING "CEGUI dependencies are not built and no python found to run build script. You need to do it manually to use CEGUI.")
	endif()
endif()

if(DEM_CEGUI_DEPS_FOUND)	
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/CEGUI" "${CMAKE_CURRENT_BINARY_DIR}/CEGUI")
	get_all_targets(DEM_CEGUI_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/CEGUI")
	foreach(_TARGET_NAME IN LISTS DEM_CEGUI_TARGETS)
		set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "CEGUI")
		target_include_directories(${_TARGET_NAME} INTERFACE
			"${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/cegui/include"
			"${CMAKE_CURRENT_BINARY_DIR}/CEGUI/cegui/include"
			"${CMAKE_CURRENT_SOURCE_DIR}/CEGUI/dependencies/include")
		if(CEGUI_BUILD_STATIC_CONFIGURATION)
			target_compile_definitions(${_TARGET_NAME} INTERFACE CEGUI_STATIC)
			if(CEGUI_BUILD_STATIC_FACTORY_MODULE)
				target_compile_definitions(${_TARGET_NAME} INTERFACE CEGUI_BUILD_STATIC_FACTORY_MODULE)
			endif()
		endif()
		if(MSVC AND DEM_DEPS_DISABLE_CXX_EXCEPTIONS)
			target_compile_options(${_TARGET_NAME} PRIVATE "/wd4530") # Don't complain about exception unwinding
		endif()
	endforeach()
endif()

# Dependency: Bullet Physics

set(BUILD_CLSOCKET OFF CACHE INTERNAL "" FORCE)
set(BUILD_ENET OFF CACHE INTERNAL "" FORCE)
set(BUILD_EXTRAS OFF CACHE INTERNAL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE INTERNAL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE INTERNAL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE INTERNAL "" FORCE)
set(BUILD_BULLET3 OFF CACHE INTERNAL "" FORCE)
set(BUILD_PYBULLET OFF CACHE INTERNAL "" FORCE)
set(USE_GLUT OFF CACHE INTERNAL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE INTERNAL "" FORCE)
set(USE_MSVC_SSE2 ON CACHE INTERNAL "" FORCE)
#USE_GRAPHICAL_BENCHMARK
#USE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD
#BT_USE_DOUBLE_PRECISION preprocessor def

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/bullet" "${CMAKE_CURRENT_BINARY_DIR}/bullet")
get_all_targets(DEM_BULLET_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/bullet")
foreach(_TARGET_NAME IN LISTS DEM_BULLET_TARGETS)
	set_target_properties(${_TARGET_NAME} PROPERTIES FOLDER "bullet")
	target_include_directories(${_TARGET_NAME} INTERFACE "${BULLET_PHYSICS_SOURCE_DIR}/src")
	target_compile_definitions(${_TARGET_NAME} PRIVATE BT_NO_PROFILE BT_USE_SSE_IN_API)
	if(MSVC AND DEM_DEPS_DISABLE_CXX_EXCEPTIONS)
		target_compile_options(${_TARGET_NAME} PRIVATE "/wd4530") # xlocale header under MSVC requires /EHsc
	endif()
endforeach()

# Dependency: Detour navigation lib + debug drawing for it

set(RECASTNAVIGATION_DEMO OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(RECASTNAVIGATION_STATIC ON CACHE INTERNAL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/Detour" "${CMAKE_CURRENT_BINARY_DIR}/recastnavigation/Detour")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DetourCrowd" "${CMAKE_CURRENT_BINARY_DIR}/recastnavigation/DetourCrowd")
set_target_properties(Detour PROPERTIES FOLDER "recastnavigation")
set_target_properties(DetourCrowd PROPERTIES FOLDER "recastnavigation")

# Instead of using RecastNavigation::DebugUtils with many dependencies, we build only required part
add_library(DetourDebugUtils STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DebugUtils/Include/DebugDraw.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DebugUtils/Include/DetourDebugDraw.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DebugUtils/Source/DebugDraw.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DebugUtils/Source/DetourDebugDraw.cpp"
)
target_include_directories(DetourDebugUtils PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/recastnavigation/DebugUtils/Include>")
target_link_libraries(DetourDebugUtils Detour)
set_target_properties(DetourDebugUtils PROPERTIES FOLDER "recastnavigation")
target_compile_definitions(DetourDebugUtils PUBLIC NO_DETOUR_TILE_CACHE_DEBUG_DRAW)

# Processing common settings for all dependency targets
# NB: CEGUI ignores DEM_DEPS_DISABLE_CXX_RTTI because it uses dynamic_cast

# HACK: empty generator expressions are used for config suffix suppression
set(DEM_DEPS_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib/$<$<BOOL:FALSE>:>")
set(DEM_DEPS_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/$<$<BOOL:FALSE>:>")

get_all_targets(DEM_DEPS_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(_TARGET_NAME IN LISTS DEM_DEPS_TARGETS)

	get_target_property(_TARGET_TYPE ${_TARGET_NAME} TYPE)
	if (${_TARGET_TYPE} IN_LIST DEM_DEPS_TARGET_TYPES_TO_EXPORT)
		list(APPEND DEM_DEPS_TARGETS_TO_EXPORT ${_TARGET_NAME})
	endif()

	if (${_TARGET_TYPE} IN_LIST DEM_DEPS_TARGET_TYPES_TO_PROCESS)

		set_target_properties(${_TARGET_NAME} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY ${DEM_DEPS_LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY ${DEM_DEPS_LIB_DIR}
			RUNTIME_OUTPUT_DIRECTORY ${DEM_DEPS_BIN_DIR}
			DEBUG_POSTFIX "_d"
		)

		foreach(_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
			string(TOUPPER "${_CONFIG}" _CONFIG)
			set_target_properties(${_TARGET_NAME} PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_LIB_DIR}
				LIBRARY_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_LIB_DIR}
				RUNTIME_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_DEPS_BIN_DIR}
			)
		endforeach()

		if(MSVC)
			if(DEM_DEPS_DISABLE_CXX_RTTI AND NOT ${_TARGET_NAME} IN_LIST DEM_CEGUI_TARGETS)
				target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GR->)
			endif()
			if(DEM_ASAN)
				target_compile_options(${_TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/fsanitize=address>)
				#!!!DBG TMP! Need to build CEGUI deps with /fsanitize=address
				target_compile_definitions(${_TARGET_NAME} PRIVATE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
			endif()
			target_compile_options(${_TARGET_NAME} PRIVATE "/Zc:__cplusplus")
		endif()

	endif()

endforeach()

# Package

#???try installable package? try relocatable package?

#???Create interface imported target with all other targets listed? No way to obtain deps list otherwise.

export(TARGETS ${DEM_DEPS_TARGETS_TO_EXPORT} NAMESPACE DEMDeps:: FILE DEMDepsConfig.cmake)


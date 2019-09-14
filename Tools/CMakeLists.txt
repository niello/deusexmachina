cmake_minimum_required(VERSION 3.8.2) # source_group TREE
project(DEM-Tools)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# We don't disable exceptions & RTTI in tools, because they aren't runtime.
# If required, sample setup can be found in engine dependencies CMakeLists.txt.

include(../CMake/DEMCMakeUtils.cmake)

#???run python script to update cmake src lists?

# Tool: ShaderCompiler

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ShaderCompiler" "${CMAKE_CURRENT_BINARY_DIR}/ShaderCompiler")

# TODO: change directories to appropriate! Each tool in its binary folder? Or all in one, but not inside build?

# HACK: empty generator expressions are used for config suffix suppression
set(DEM_TOOLS_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib/$<$<BOOL:FALSE>:>")
set(DEM_TOOLS_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/$<$<BOOL:FALSE>:>")

get_all_targets(DEM_TOOLS_TARGETS "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(_TARGET_NAME IN LISTS DEM_TOOLS_TARGETS)

	set_target_properties(${_TARGET_NAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${DEM_TOOLS_LIB_DIR}
		LIBRARY_OUTPUT_DIRECTORY ${DEM_TOOLS_LIB_DIR}
		RUNTIME_OUTPUT_DIRECTORY ${DEM_TOOLS_BIN_DIR}
		DEBUG_POSTFIX "_d"
	)

	foreach(_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
		string(TOUPPER "${_CONFIG}" _CONFIG)
		set_target_properties(${_TARGET_NAME} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_TOOLS_LIB_DIR}
			LIBRARY_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_TOOLS_LIB_DIR}
			RUNTIME_OUTPUT_DIRECTORY_${_CONFIG} ${DEM_TOOLS_BIN_DIR}
		)
	endforeach()

endforeach()

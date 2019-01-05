# This is NOT an original CMakeLists.txt which can be found at https://github.com/leethomason/tinyxml2
# DeusExMachina team provides this own minimal CMakeLists.txt to minimize engine dependency weight and setup effort.
# Source and header moved to another "tinyxml2" folder for external code to use #include <tinyxml2/tinyxml2.h>
# Install mode is not supported yet but will be added in the future.
cmake_minimum_required(VERSION 3.0.2)

set(GENERIC_LIB_VERSION "7.0.1")
set(GENERIC_LIB_SOVERSION "7")

#option(BUILD_SHARED_LIBS "build as shared library" ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(tinyxml2 tinyxml2/tinyxml2.cpp tinyxml2/tinyxml2.h)

set_target_properties(tinyxml2 PROPERTIES
	VERSION "${GENERIC_LIB_VERSION}"
	SOVERSION "${GENERIC_LIB_SOVERSION}"
	DEBUG_POSTFIX "_d"
)

target_include_directories(tinyxml2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2")
target_compile_definitions(tinyxml2 PUBLIC $<$<CONFIG:Debug>:TINYXML2_DEBUG>)
if(MSVC)
	target_compile_definitions(tinyxml2 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

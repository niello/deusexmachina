cmake_minimum_required(VERSION 3.5)
project (GLTFSDK)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -DFEATURE_ASSERTS_ENABLED")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11)

if (WIN32)
    # Multithreaded Build
    add_compile_options(
        /MP
    )
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Build/CMake/Modules")

if(NOT IOS)
    # This variable is set by ios.toolchain.cmake.
    # Notice that External/RapidJSON uses `find_package` for external project is ready.
    # We have to prevent modification of RapidJSON_XXX variables in cache at this moment
    find_package(RapidJSON CONFIG)
endif()
if(NOT RapidJSON_FOUND)
    add_subdirectory(External/RapidJSON)
endif()

include(GLTFPlatform)
GetGLTFPlatform(Platform)

file(GLOB source_files
    "${CMAKE_CURRENT_LIST_DIR}/Source/*.cpp"
)

file(GLOB schema_deps
    "${CMAKE_CURRENT_LIST_DIR}/schema/*"
)

find_program(POWERSHELL_PATH NAMES pwsh powershell NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/GeneratedFiles/SchemaJson.h
    COMMAND ${POWERSHELL_PATH} -ExecutionPolicy Bypass "${CMAKE_CURRENT_LIST_DIR}/GenerateSchemaJsonHeader.ps1" -outPath "${CMAKE_BINARY_DIR}/GeneratedFiles"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS "${schema_deps}"
)

add_library(GLTFSDK ${source_files} ${CMAKE_BINARY_DIR}/GeneratedFiles/SchemaJson.h)

if (MSVC)
    # Generate PDB files in all configurations, not just Debug (/Zi)
    # Set warning level to 4 (/W4)
    target_compile_options(GLTFSDK PRIVATE "/Zi;/W4;/EHsc")

    if((MSVC_VERSION GREATER 1920) AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
        target_compile_options(GLTFSDK PRIVATE "$<$<CONFIG:RELEASE>:/guard:ehcont>")
        target_link_options(GLTFSDK PRIVATE "$<$<CONFIG:RELEASE>:/guard:ehcont>")
    endif()

    # Make sure that all PDB files on Windows are installed to the output folder with the libraries. By default, only the debug build does this.
    set_target_properties(GLTFSDK PROPERTIES COMPILE_PDB_NAME "GLTFSDK" COMPILE_PDB_OUTPUT_DIRECTORY "${LIBRARY_OUTPUT_DIRECTORY}")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(GLTFSDK
        PRIVATE "-Wunguarded-availability"
        PRIVATE "-Wall"
        PRIVATE "-Werror"
        PUBLIC "-Wno-unknown-pragmas")
endif()

target_include_directories(GLTFSDK
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/Inc"
    PRIVATE "${CMAKE_SOURCE_DIR}/Built/Int"
    PRIVATE "${CMAKE_BINARY_DIR}/GeneratedFiles"
)

if(RapidJSON_FOUND)
    target_include_directories(GLTFSDK
        PUBLIC $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIRS}>
    )
elseif(TARGET RapidJSON)
    get_target_property(RapidJSON_INCLUDE_DIRS RapidJSON INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(GLTFSDK
        PUBLIC $<BUILD_INTERFACE:${RapidJSON_INCLUDE_DIRS}>
    )
endif()

CreateGLTFInstallTargets(GLTFSDK ${Platform})

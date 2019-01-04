cmake_minimum_required(VERSION 3.0.2)
project(theora)

# Required modules
include(GNUInstallDirs)
include(CheckIncludeFiles)

# Build options
#option(BUILD_SHARED_LIBS "Build shared library" OFF)
#if(APPLE)
#    option(BUILD_FRAMEWORK "Build Framework bundle for OSX" OFF)
#endif()

#if(BUILD_FRAMEWORK)
#	set(BUILD_SHARED_LIBS TRUE)
#endif()

option(THEORA_BUILD_ENCODER "Build theora encoder" OFF)

# Extract project version from configure.ac
file(READ configure.ac CONFIGURE_AC_CONTENTS)
string(REGEX MATCH "AC_INIT\\(\\[libtheora\\],\\[([0-9]*).([0-9]*).([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Helper function to get version-info
function(get_version_info result current_var_name age_var_name revision_var_name)
    string(REGEX MATCH "${current_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_CURRENT ${CMAKE_MATCH_1})

    string(REGEX MATCH "${age_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_AGE ${CMAKE_MATCH_1})

    string(REGEX MATCH "${revision_var_name}=([0-9]*)" DUMMY ${CONFIGURE_AC_CONTENTS})
    set(VERSION_INFO_REVISION ${CMAKE_MATCH_1})

    math(EXPR VERSION_INFO_CURRENT_MINUS_AGE "${VERSION_INFO_CURRENT} - ${VERSION_INFO_AGE}")

    set(${result} "${VERSION_INFO_CURRENT_MINUS_AGE}.${VERSION_INFO_AGE}.${VERSION_INFO_REVISION}" PARENT_SCOPE)
endfunction()

# Helper function to configure pkg-config files
function(configure_pkg_config_file pkg_config_file_in)
    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix ${CMAKE_INSTALL_FULL_BINDIR})
    set(libdir ${CMAKE_INSTALL_FULL_LIBDIR})
    set(includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
    set(VERSION ${PROJECT_VERSION})
    string(REPLACE ".in" "" pkg_config_file ${pkg_config_file_in})
    configure_file(${pkg_config_file_in} ${pkg_config_file} @ONLY)
endfunction()

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# Find ogg dependency
if(NOT OGG_ROOT)
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_OGG QUIET ogg)
    find_path(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${PC_OGG_INCLUDE_DIRS} PATH_SUFFIXES ogg)
    #find_library(OGG_LIBRARIES NAMES ogg HINTS ${PC_OGG_LIBRARY_DIRS})
else()
    find_path(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${OGG_ROOT}/include PATH_SUFFIXES ogg)
    #find_library(OGG_LIBRARIES NAMES ogg HINTS ${OGG_ROOT}/lib ${OGG_ROOT}/lib64)
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OGG DEFAULT_MSG OGG_INCLUDE_DIRS OGG_LIBRARIES)

set(THEORA_PUBLIC_HEADERS
	include/theora/codec.h
	include/theora/theora.h
	include/theora/theoradec.h
)

set(THEORA_HEADERS
	lib/apiwrapper.h
	lib/bitpack.h
	lib/collect.h
	lib/dct.h
	lib/decint.h
	lib/dequant.h
	lib/encint.h
	lib/enquant.h
	lib/huffdec.h
	lib/huffenc.h
	lib/huffman.h
	lib/internal.h
	lib/mathops.h
	lib/modedec.h
	lib/ocintrin.h
	lib/quant.h
	lib/state.h
)

set(THEORA_SOURCES
	lib/apiwrapper.c
	lib/bitpack.c
	lib/collect.c
	lib/decapiwrapper.c
	lib/decinfo.c
	lib/decode.c
	lib/dequant.c
	lib/fragment.c
	lib/huffdec.c
	lib/idct.c
	lib/info.c
	lib/internal.c
	lib/quant.c
	lib/state.c
)

if(MSVC)
    list(APPEND THEORA_HEADERS
		lib/x86_vc/mmxloop.h
		lib/x86_vc/x86cpu.h
		lib/x86_vc/x86enc.h
		lib/x86_vc/x86int.h
		lib/x86_vc/x86zigzag.h
	)
    list(APPEND THEORA_SOURCES
		lib/x86_vc/mmxfrag.c
		lib/x86_vc/mmxidct.c
		lib/x86_vc/mmxstate.c
		lib/x86_vc/x86cpu.c
		lib/x86_vc/x86state.c
	)
else() 
    list(APPEND THEORA_HEADERS
		lib/x86/mmxloop.h
		lib/x86/x86cpu.h
		lib/x86/x86enc.h
		lib/x86/x86int.h
		lib/x86/x86zigzag.h
		lib/x86/sse2trans.h
	)
    list(APPEND THEORA_SOURCES
		lib/x86/mmxfrag.c
		lib/x86/mmxidct.c
		lib/x86/mmxstate.c
		lib/x86/x86cpu.c
		lib/x86/x86state.c
		lib/x86/sse2idct.c
	)
endif()

if(THEORA_BUILD_ENCODER)
	list(APPEND THEORA_PUBLIC_HEADERS
		include/theora/theoraenc.h
	)

	set(THEORA_ENCODER_SOURCES
		lib/analyze.c
		lib/encapiwrapper.c
		lib/encfrag.c
		lib/encinfo.c
		lib/encode.c
		lib/enquant.c
		lib/fdct.c
		lib/huffenc.c
		lib/mathops.c
		lib/mcenc.c
		lib/rate.c
		lib/tokenize.c
		lib/x86/sse2encfrag.c
		lib/x86/x86enquant.c
	)

	# x64 detection hack
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		list(APPEND THEORA_ENCODER_SOURCES
			x86/sse2fdct.c
		)
	endif()

	if(MSVC)
		list(APPEND THEORA_ENCODER_SOURCES
			lib/x86_vc/mmxencfrag.c
			lib/x86_vc/mmxfdct.c
			lib/x86_vc/x86enc.c
		)
	else() 
		list(APPEND THEORA_ENCODER_SOURCES
			lib/x86/mmxencfrag.c
			lib/x86/mmxfdct.c
			lib/x86/x86enc.c
		)
	endif()
else()
	set(THEORA_ENCODER_SOURCES
		lib/encoder_disabled.c
	)
endif()

include_directories(include)
include_directories(lib)
include_directories(${OGG_INCLUDE_DIRS})

add_library(theora STATIC ${THEORA_HEADERS} ${THEORA_SOURCES} ${THEORA_ENCODER_SOURCES})
target_include_directories(theora PUBLIC include ${OGG_INCLUDE_DIRS})
#add_library(theoradec ${VORBISFILE_SOURCES})

get_version_info(THEORA_VERSION_INFO "TH_LIB_CURRENT" "TH_LIB_AGE" "TH_LIB_REVISION")
set_target_properties(theora PROPERTIES SOVERSION ${THEORA_VERSION_INFO})
#get_version_info(THEORADEC_VERSION_INFO "THDEC_LIB_CURRENT" "THDEC_LIB_AGE" "THDEC_LIB_REVISION")
#set_target_properties(theoradec PROPERTIES SOVERSION ${THEORADEC_VERSION_INFO})

#target_link_libraries(theora ${OGG_LIBRARIES})
#target_link_libraries(theoradec theora)

install(FILES ${THEORA_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/theora)

install(TARGETS theora     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install(TARGETS theoradec  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION #${CMAKE_INSTALL_LIBDIR} FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})

configure_pkg_config_file(theora.pc.in)
configure_pkg_config_file(theoradec.pc.in)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/theora.pc
    ${CMAKE_CURRENT_BINARY_DIR}/theoradec.pc
    DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

if(THEORA_BUILD_ENCODER)
	#add_library(theoraenc ${VORBISENC_SOURCES})

	#get_version_info(THEORAENC_VERSION_INFO "THENC_LIB_CURRENT" "THENC_LIB_AGE" "THENC_LIB_REVISION")
	#set_target_properties(theoraenc PROPERTIES SOVERSION ${THEORAENC_VERSION_INFO})

	#target_link_libraries(theoraenc theora)

	#install(TARGETS theoraenc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION #${CMAKE_INSTALL_LIBDIR} FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})

	configure_pkg_config_file(theoraenc.pc.in)

	install(
		FILES
		${CMAKE_CURRENT_BINARY_DIR}/theoraenc.pc
		DESTINATION
		${CMAKE_INSTALL_LIBDIR}/pkgconfig
	)
endif()

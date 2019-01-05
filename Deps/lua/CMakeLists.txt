cmake_minimum_required(VERSION 3.8.2) # source_group TREE

include(src.cmake)
list(REMOVE_ITEM LUA_SOURCES src/lua.c) # remove interpreter from the list of all sources
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${LUA_HEADERS} ${LUA_SOURCES})
add_library(lua ${LUA_HEADERS} ${LUA_SOURCES})
target_include_directories(lua PUBLIC src)
target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)

cmake_minimum_required(VERSION 3.8.2) # source_group TREE

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES src/sqlite3.h src/sqlite3.c)
add_library(sqlite3 src/sqlite3.h src/sqlite3.c)

set_target_properties(sqlite3 PROPERTIES DEBUG_POSTFIX "_d")

target_include_directories(sqlite3 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")

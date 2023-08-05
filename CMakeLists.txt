cmake_minimum_required(VERSION 3.10)

project(export_ibook_notes)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

# 查找 libsqlite3 库
find_library(SQLITE3_LIB sqlite3)
message(STATUS "SQLite3 Library Path: ${SQLITE3_LIB}")


add_executable(export_ibook_notes src/export_ibook_notes.cpp)
target_link_libraries(export_ibook_notes sqlite3)



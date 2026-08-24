cmake_minimum_required(VERSION 4.4.0)
project(Mycraft LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(klibrary)

file(GLOB_RECURSE mycraft_sources CONFIGURE_DEPENDS source/*.cpp)
add_executable(mycraft ${mycraft_sources})
target_link_libraries(mycraft PUBLIC klibrarylib)
target_include_directories(mycraft PUBLIC source)
set_target_properties(mycraft PROPERTIES
    DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/workdir)

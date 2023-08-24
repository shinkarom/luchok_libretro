cmake_minimum_required(VERSION 3.9)
set(LIBNAME luchok_libretro)
project(${LIBNAME} LANGUAGES C CXX VERSION 1.0.1 DESCRIPTION "Luchok fantasy console, Libretro core")
set(CMAKE_SHARED_LIBRARY_PREFIX )

add_library(${LIBNAME} SHARED 
    src/libretro.cpp
    )
set_target_properties(${LIBNAME} PROPERTIES
    SOVERSION 1
    PUBLIC_HEADER src/libretro.h
    LINKER_LANGUAGE CXX)
#target_link_libraries(${LIBNAME} )
target_include_directories(${LIBNAME} PUBLIC src)

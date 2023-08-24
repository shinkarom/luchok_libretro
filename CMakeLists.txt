cmake_minimum_required(VERSION 3.9)
set(LIBNAME libretrocore)
project(${LIBNAME} LANGUAGES C CXX VERSION 1.0.1 DESCRIPTION "Libretro core")

add_library(${LIBNAME} SHARED 
    src/libretro.cpp
    )
set_target_properties(${LIBNAME} PROPERTIES
    SOVERSION 1
    PUBLIC_HEADER src/libretro.h
    LINKER_LANGUAGE CXX)
#target_link_libraries(${LIBNAME} )
target_include_directories(${LIBNAME} PUBLIC src)

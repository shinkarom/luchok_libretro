# CMake 3.20 is required for proper C++23 support
cmake_minimum_required(VERSION 3.20)

set(LIBNAME luchok_libretro)
project(${LIBNAME} LANGUAGES C CXX VERSION 1.0.1 DESCRIPTION "Luchok fantasy console, Libretro core")

# Modern C++ settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions (e.g., gnu++23)

# Remove the "lib" prefix so the output is "luchok_libretro.so/dll"
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# --- LUA TARGET ---
set(LUA_DIR vendor/lua)
set(LUA_SRC
    ${LUA_DIR}/lapi.c ${LUA_DIR}/lcode.c ${LUA_DIR}/lctype.c ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c ${LUA_DIR}/ldump.c ${LUA_DIR}/lfunc.c ${LUA_DIR}/lgc.c
    ${LUA_DIR}/llex.c ${LUA_DIR}/lmem.c ${LUA_DIR}/lobject.c ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/lparser.c ${LUA_DIR}/lstate.c ${LUA_DIR}/lstring.c ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltm.c ${LUA_DIR}/lundump.c ${LUA_DIR}/lvm.c ${LUA_DIR}/lzio.c
    ${LUA_DIR}/lauxlib.c ${LUA_DIR}/lbaselib.c ${LUA_DIR}/lcorolib.c ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/liolib.c ${LUA_DIR}/lmathlib.c ${LUA_DIR}/loslib.c ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltablib.c ${LUA_DIR}/lutf8lib.c ${LUA_DIR}/loadlib.c ${LUA_DIR}/linit.c
)

add_library(lua STATIC ${LUA_SRC})

# MUST be ON when linking a STATIC library into a SHARED library
set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(lua PRIVATE LUA_COMPAT_5_4)
# Changed to PUBLIC so both Lua and the Libretro core can find Lua headers
target_include_directories(lua PUBLIC ${LUA_DIR})


# --- LIBRETRO CORE TARGET ---
# MODULE is technically more correct than SHARED for plugins loaded via dlopen(), 
# but SHARED works perfectly fine for Libretro cores.
add_library(${LIBNAME} SHARED src/libretro.cpp)
if(MINGW)
    target_link_options(${LIBNAME} PRIVATE "-static-libgcc" "-static-libstdc++" "-Wl,-Bstatic" "-lstdc++" "-lpthread" "-Wl,-Bdynamic")
endif()
set_target_properties(${LIBNAME} PROPERTIES
    # Output to a bin folder inside the build directory (Out-of-source builds preferred)
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin" # For Windows .dll
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin" # For Linux/macOS .so/.dylib
)

# Use PRIVATE to ensure dependencies don't leak out of this target
target_link_libraries(${LIBNAME} PRIVATE lua)
target_include_directories(${LIBNAME} PRIVATE src)
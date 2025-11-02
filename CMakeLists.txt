cmake_minimum_required(VERSION 3.10)

# Projektname dynamisch aus dem Ordnernamen ableiten
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME} C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# --- Build-Optionen ---

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type: Debug, ASan, Release" FORCE)
endif()

# Option zum Aktivieren/Deaktivieren von SIMD
option(USE_SIMD "Enable SIMD-specific optimizations" ON)


# --- Globale Compiler-Flags ---

add_compile_options(-fdiagnostics-color=never)

# Compiler warnings and build-type specific flags (for GCC/Clang)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Debug build: Enabling debug symbols for GDB.")
        add_compile_definitions(DEBUG)
        add_compile_options(-g -O0)

    elseif(CMAKE_BUILD_TYPE STREQUAL "ASan")
        message(STATUS "ASan build: Enabling sanitizers and debug symbols.")
        add_compile_definitions(DEBUG)
        add_compile_options(-fsanitize=address,leak,undefined -fno-omit-frame-pointer -g -O1)
        add_link_options(-fsanitize=address,leak,undefined)

    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Release build: Enabling optimizations.")
        add_compile_options(-O3 -DNDEBUG)

    endif()
endif()

# Architektur-spezifische SIMD-Flags hinzufügen, wenn die Option aktiviert ist
if(USE_SIMD)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        message(STATUS "x86_64 architecture detected, enabling SSE4.2")
        add_compile_options(-msse4.2)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        message(STATUS "AArch64 architecture detected, NEON is enabled by default.")
    endif()
endif()

add_compile_definitions(_XOPEN_SOURCE=700)

# --- Bibliotheken und Unterverzeichnisse ---

add_subdirectory(common)


file(GLOB_RECURSE str8_SRC CONFIGURE_DEPENDS "src/*.c" "src/*/*.c")

add_library(str8 STATIC ${str8_SRC})
target_link_libraries(str8 PRIVATE utf8_helper)
target_include_directories(str8 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# CTest-Unterstützung aktivieren
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    enable_testing()
    # --- Add tests ---
    add_subdirectory(tests)
endif()

# --- Add tools ---
add_subdirectory(research)

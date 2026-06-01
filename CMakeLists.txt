cmake_minimum_required(VERSION 3.25)

project(doritos_port LANGUAGES CXX)

option(DORITOS_DISABLE_NETWORKING "Compile with networking disabled by default" ON)
option(DORITOS_TRACE_IMPORTS "Enable extra import tracing in handwritten code" ON)
option(DORITOS_REQUIRE_GENERATED "Fail configure if ReXGlue generated sources are absent" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DORITOS_REXGLUE_ROOT "K:/1943/RexGlueCurrent" CACHE PATH
    "ReXGlue SDK root. Override with a local copied RexGlueCurrent when patching the SDK.")

if(NOT EXISTS "${DORITOS_REXGLUE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "ReXGlue SDK not found at '${DORITOS_REXGLUE_ROOT}'. "
        "Set DORITOS_REXGLUE_ROOT to a copied or known-good ReXGlue checkout.")
endif()

add_subdirectory("${DORITOS_REXGLUE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")

add_library(DORITOS_logging STATIC
    src/logging/log.cpp
)
target_include_directories(DORITOS_logging PUBLIC src)

add_library(DORITOS_config STATIC
    src/config/port_config.cpp
)
target_include_directories(DORITOS_config PUBLIC src)
target_link_libraries(DORITOS_config PUBLIC DORITOS_logging)

add_library(DORITOS_vfs STATIC
    src/vfs/mount_table.cpp
)
target_include_directories(DORITOS_vfs PUBLIC src)
target_link_libraries(DORITOS_vfs PUBLIC DORITOS_config DORITOS_logging)

add_library(DORITOS_platform STATIC
    src/platform/windows_platform.cpp
)
target_include_directories(DORITOS_platform PUBLIC src)

add_library(DORITOS_runtime STATIC
    src/runtime/import_trap.cpp
    src/runtime/runtime_plan.cpp
    src/runtime/xam_profile_overrides.cpp
)
target_include_directories(DORITOS_runtime PUBLIC src)
target_link_libraries(DORITOS_runtime PUBLIC DORITOS_logging rex::system)

add_library(DORITOS_graphics STATIC
    src/graphics/backend.cpp
)
target_include_directories(DORITOS_graphics PUBLIC src)
target_link_libraries(DORITOS_graphics PUBLIC DORITOS_logging)

add_library(DORITOS_audio STATIC
    src/audio/backend.cpp
)
target_include_directories(DORITOS_audio PUBLIC src)
target_link_libraries(DORITOS_audio PUBLIC DORITOS_logging)

add_library(DORITOS_input STATIC
    src/input/mapping.cpp
)
target_include_directories(DORITOS_input PUBLIC src)

add_library(DORITOS_net STATIC
    src/net/network_mode.cpp
)
target_include_directories(DORITOS_net PUBLIC src)
target_link_libraries(DORITOS_net PUBLIC DORITOS_logging)

add_library(DORITOS_diagnostics STATIC
    src/diagnostics/crash_dumps.cpp
)
target_include_directories(DORITOS_diagnostics PUBLIC src)
target_link_libraries(DORITOS_diagnostics PUBLIC DORITOS_logging)
if(WIN32)
    target_link_libraries(DORITOS_diagnostics PRIVATE Dbghelp)
endif()

set(DORITOS_APP_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(doritos_port WIN32 ${DORITOS_APP_SOURCES})
else()
    add_executable(doritos_port ${DORITOS_APP_SOURCES})
endif()

target_include_directories(doritos_port PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/port/generated"
)

target_link_libraries(doritos_port PRIVATE
    DORITOS_logging
    DORITOS_config
    DORITOS_vfs
    DORITOS_platform
    DORITOS_runtime
    DORITOS_graphics
    DORITOS_audio
    DORITOS_input
    DORITOS_net
    DORITOS_diagnostics
    rex::core
    rex::ui
    rex::system
    rex::kernel
    rex::graphics
    rex::audio
    rex::input
)

set(DORITOS_GENERATED_SOURCES_CMAKE
    "${CMAKE_CURRENT_SOURCE_DIR}/port/generated/sources.cmake")

if(EXISTS "${DORITOS_GENERATED_SOURCES_CMAKE}")
    include("${DORITOS_GENERATED_SOURCES_CMAKE}")
    target_sources(doritos_port PRIVATE ${GENERATED_SOURCES})
    target_compile_definitions(doritos_port PRIVATE DORITOS_HAS_REXGLUE_GENERATED=1)
    set(DORITOS_GENERATED_COMPILE_OPTIONS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND DORITOS_GENERATED_COMPILE_OPTIONS "-Wno-unused-label")
    elseif(MSVC)
        list(APPEND DORITOS_GENERATED_COMPILE_OPTIONS "/wd4102")
    endif()
    set(DORITOS_XAM_PROFILE_REDIRECT
        "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/xam_profile_import_redirect.h")
    if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
                CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
        list(APPEND DORITOS_GENERATED_COMPILE_OPTIONS "/FI${DORITOS_XAM_PROFILE_REDIRECT}")
    else()
        list(APPEND DORITOS_GENERATED_COMPILE_OPTIONS "-include" "${DORITOS_XAM_PROFILE_REDIRECT}")
    endif()
    if(DORITOS_GENERATED_COMPILE_OPTIONS)
        set_source_files_properties(${GENERATED_SOURCES}
            PROPERTIES COMPILE_OPTIONS "${DORITOS_GENERATED_COMPILE_OPTIONS}")
    endif()
    if(DEFINED GENERATED_USES_SEH AND GENERATED_USES_SEH)
        if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
                    CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
            target_compile_options(doritos_port PRIVATE /EHa)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(doritos_port PRIVATE -fexceptions)
        endif()
    endif()
else()
    if(DORITOS_REQUIRE_GENERATED)
        message(FATAL_ERROR
            "Generated sources are missing. Run tools/codegen.ps1 first.")
    endif()
    target_compile_definitions(doritos_port PRIVATE DORITOS_HAS_REXGLUE_GENERATED=0)
endif()

target_compile_definitions(doritos_port PRIVATE
    DORITOS_PLATFORM_WINDOWS=$<BOOL:${WIN32}>
    DORITOS_DISABLE_NETWORKING=$<BOOL:${DORITOS_DISABLE_NETWORKING}>
    DORITOS_TRACE_IMPORTS=$<BOOL:${DORITOS_TRACE_IMPORTS}>
)

if(MSVC)
    target_compile_options(doritos_port PRIVATE /W4 /permissive-)
else()
    target_compile_options(doritos_port PRIVATE -Wall -Wextra)
endif()

rexglue_configure_target(doritos_port)

add_custom_target(DORITOS_codegen
    COMMAND "$<TARGET_FILE:rex::rexglue>" codegen
            "${CMAKE_CURRENT_SOURCE_DIR}/port/rexglue-config/doritos_port.toml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Running local ReXGlue codegen for Doritos Crash Course"
    VERBATIM
)


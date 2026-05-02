cmake_minimum_required(VERSION 4.1)
project(jetbrains_dev LANGUAGES CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INJECT_SOURCES
        src/library.cpp
        src/logger/Logger.cpp
        src/titan_hook.h
        src/winmm.cpp
        src/winmm_jump.asm
        src/hooks/hook_manager.cpp
        src/hooks/hook_manager.h
        src/hooks/hook.cpp
        src/hooks/hook.h
        src/utils/console.h
        src/utils/pattern.cpp
        src/utils/pattern.h
        src/hooks/impl/load_library_hook.h
        src/features/feature.cpp
        src/features/feature.h
        src/features/feature_manager.cpp
        src/features/feature_manager.h
        src/utils/jvm_utility.cpp
        src/utils/jvm_utility.h
        src/environment.h
        src/utils/guards.h
        src/utils/converts.h
        src/utils/converts.h
        src/features/impl/crash_logger.cpp
        src/features/impl/crash_logger.h
        src/utils/hex.h
        src/hooks/impl/load_library_hook.cpp
)

add_library(winmm SHARED ${INJECT_SOURCES})

# Force-compile .h files listed as ClCompile in the original vcxproj
set_source_files_properties(
        ${WINMM_FORCE_COMPILE_HEADERS}
        PROPERTIES HEADER_FILE_ONLY FALSE COMPILE_FLAGS "/TP"
)

# ============================================================
# Include directories
# ============================================================
target_include_directories(winmm PRIVATE
        src
        third_party/detour/include
        third_party/java/include
)

target_compile_definitions(winmm PRIVATE
        TITANMODLOADER_EXPORTS
        _WINDOWS
        _USRDLL
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
)


# ============================================================
# Use dynamic CRT (/MD for Release, /MDd for Debug) to match pre-built libs
# ============================================================
set_property(TARGET winmm PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# ============================================================
# Compile options
# ============================================================
if(MSVC)
    # Common
    target_compile_options(winmm PRIVATE
            /W3 /MP /Gm- /Zc:inline /EHsc /permissive- /source-charset:utf-8
    )
    # Release-specific
    target_compile_options(winmm PRIVATE
            $<$<CONFIG:Release>:/O2 /Oi /GL /Gy /DNDEBUG>
    )
    # Debug-specific
    target_compile_options(winmm PRIVATE
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    )
    # MASM options - use dedicated flags to avoid inheriting C++ flags
    set_source_files_properties(tracer/winmm_jump.asm PROPERTIES
            COMPILE_FLAGS "/c /Cp"
    )
endif()

# ============================================================
# Link options
# ============================================================
if(MSVC)
    target_link_options(winmm PRIVATE
            /SUBSYSTEM:WINDOWS
            /MACHINE:X64
            $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
            $<$<CONFIG:Debug>:/DEBUG>
    )
endif()

# ============================================================
# Link libraries
# ============================================================

# Pre-built static libs from third_party
target_link_libraries(winmm PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/detour/detours.lib
)

# ============================================================
# Output: winmm.dll
# ============================================================
set_target_properties(winmm PROPERTIES
        OUTPUT_NAME "winmm"
        PREFIX ""
        SUFFIX ".dll"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
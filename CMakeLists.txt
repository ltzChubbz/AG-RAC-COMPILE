cmake_minimum_required(VERSION 3.26)
project(ag_rac C)

# ─── Build target selection ────────────────────────────────────────────────────
# Usage:
#   cmake -B build_mips -DBUILD_TARGET=mips   # Cross-compile to MIPS for SHA1 matching
#   cmake -B build_pc   -DBUILD_TARGET=pc     # Native PC build with HAL layer
set(BUILD_TARGET "pc" CACHE STRING "Build target: 'pc' or 'mips'")
set_property(CACHE BUILD_TARGET PROPERTY STRINGS pc mips)

message(STATUS "AG-RAC build target: ${BUILD_TARGET}")

# ─── Source files (shared between both targets) ────────────────────────────────
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "src/engine/*.c"
)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "src/game/*.c"
)
file(GLOB_RECURSE AUDIO_SOURCES CONFIGURE_DEPENDS
    "src/audio/*.c"
)
file(GLOB_RECURSE RENDERER_SOURCES CONFIGURE_DEPENDS
    "src/renderer/*.c"
)

set(GAME_SRC
    ${ENGINE_SOURCES}
    ${GAME_SOURCES}
    ${AUDIO_SOURCES}
    ${RENDERER_SOURCES}
)

# ─── Include paths ─────────────────────────────────────────────────────────────
include_directories(include)

# ─── MIPS target — matching decompilation build ────────────────────────────────
if(BUILD_TARGET STREQUAL "mips")
    message(STATUS "Configuring MIPS cross-compilation target (matching decompilation)")

    # Requires mipsel-linux-gnu-gcc in PATH (install via WSL: apt install gcc-mipsel-linux-gnu)
    set(CMAKE_C_COMPILER "mipsel-linux-gnu-gcc")
    set(CMAKE_SYSTEM_NAME "Generic")
    set(CMAKE_SYSTEM_PROCESSOR "mips")

    # Flags designed to match the original PS2 SDK compiler (SN Systems ProDG)
    # These will be refined as we identify the exact compiler version used.
    set(MIPS_FLAGS
        "-march=r5900"          # Emotion Engine (extended MIPS III)
        "-mabi=eabi"
        "-EL"                   # Little-endian
        "-fno-builtin"
        "-fno-exceptions"
        "-O2"                   # Will be tuned per-function
        "-G0"                   # No GP-relative addressing (matches PS2 SDK default)
        "-mno-gpopt"
    )
    string(JOIN " " MIPS_FLAGS_STR ${MIPS_FLAGS})

    set(CMAKE_C_FLAGS "${MIPS_FLAGS_STR}")

    add_executable(rac_mips ${GAME_SRC})
    target_compile_definitions(rac_mips PRIVATE TARGET_PS2=1)

    # SHA1 comparison against the original ELF after build
    # Set ORIGINAL_ELF to your extracted ELF path
    set(ORIGINAL_ELF "${CMAKE_SOURCE_DIR}/extracted/SCUS_971.99"
        CACHE FILEPATH "Path to the original extracted game ELF for SHA1 comparison")

    if(EXISTS "${ORIGINAL_ELF}")
        add_custom_command(TARGET rac_mips POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running SHA1 comparison..."
            COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/verify/sha1_compare.py"
                    "${ORIGINAL_ELF}"
                    "$<TARGET_FILE:rac_mips>"
            COMMENT "Verifying compiled output matches original ELF"
        )
    else()
        message(WARNING "ORIGINAL_ELF not found at '${ORIGINAL_ELF}'. SHA1 verification disabled.")
        message(WARNING "Run 'python tools/iso_extract/iso_extract.py' first to extract your ISO.")
    endif()

# ─── PC target — native build with PS2 HAL ────────────────────────────────────
elseif(BUILD_TARGET STREQUAL "pc")
    message(STATUS "Configuring native PC build target")

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    # PC HAL source files
    file(GLOB_RECURSE HAL_SOURCES CONFIGURE_DEPENDS "pc/hal/*.c")
    file(GLOB_RECURSE WINDOW_SOURCES CONFIGURE_DEPENDS "pc/window/*.c")

    set(PC_SRC
        ${GAME_SRC}
        ${HAL_SOURCES}
        ${WINDOW_SOURCES}
    )

    add_executable(rac_pc ${PC_SRC})
    target_compile_definitions(rac_pc PRIVATE TARGET_PC=1)
    target_include_directories(rac_pc PRIVATE pc)

    # ── SDL2 ──────────────────────────────────────────────────────────────────
    # Try a local install first; fall back to FetchContent if not found.
    find_package(SDL2 QUIET)
    if(NOT SDL2_FOUND)
        message(STATUS "SDL2 not found locally — fetching via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            SDL2
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG        release-2.30.10
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(SDL2)
        # FetchContent exposes SDL2::SDL2 as a target directly
        target_link_libraries(rac_pc PRIVATE SDL2::SDL2)
    else()
        target_include_directories(rac_pc PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(rac_pc PRIVATE ${SDL2_LIBRARIES})
    endif()

    # ── OpenGL ────────────────────────────────────────────────────────────────
    find_package(OpenGL REQUIRED)
    target_link_libraries(rac_pc PRIVATE OpenGL::GL)

    # ── Assets path ───────────────────────────────────────────────────────────
    set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets" CACHE PATH "Path to extracted game assets")
    target_compile_definitions(rac_pc PRIVATE
        ASSETS_PATH="${ASSETS_PATH}"
    )

    # Windows-specific settings
    if(WIN32)
        target_link_libraries(rac_pc PRIVATE ws2_32)
        # Use SDL2 subsystem (no console window in release)
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            set_target_properties(rac_pc PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
    endif()

    # Compiler warnings
    if(MSVC)
        target_compile_options(rac_pc PRIVATE /W4)
    else()
        target_compile_options(rac_pc PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

else()
    message(FATAL_ERROR "Unknown BUILD_TARGET '${BUILD_TARGET}'. Use -DBUILD_TARGET=pc or -DBUILD_TARGET=mips")
endif()

# ─── Custom targets for tooling ────────────────────────────────────────────────
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    # Extract ISO
    add_custom_target(extract_iso
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/iso_extract/iso_extract.py"
            --help
        COMMENT "Run 'python tools/iso_extract/iso_extract.py --iso <path> --out extracted/' to extract your ISO"
        VERBATIM
    )

    # Show ELF info
    add_custom_target(elf_info
        COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/elf_info/elf_info.py"
            --elf "${CMAKE_SOURCE_DIR}/extracted/SCUS_971.99"
        COMMENT "Displaying ELF section info"
        VERBATIM
    )
endif()

################################################################################
#
#   Don't directly run cmake, use make!
#   Example commands:
#       make
#       make help
#       make build
#       make run
#       make clean
#
################################################################################

cmake_minimum_required(VERSION 3.24)

# === Checking that all options are correct ===
if (NOT ("${BUILD_TYPE}" STREQUAL "Debug" OR "${BUILD_TYPE}" STREQUAL "Release"))
    message(FATAL_ERROR "Unknown BUILD_TYPE \"${BUILD_TYPE}\", please set BUILD_TYPE to \"Debug\" or \"Release\"")
else()
    set(CMAKE_BUILD_TYPE "${BUILD_TYPE}")
endif()

# --- OS specific configuration ---
# APPLE and BSD to clang, cause this is how it's done in the official raylib template
# !You can't use APPLE/BSD/LINUX before project() call, but if you set compiler after
# project call on some systems CMake may not find the compiler
# if (APPLE OR BSD)
#     set(CMAKE_C_COMPILER   clang)
#     set(CMAKE_CXX_COMPILER clang++)
# else()
    set(CMAKE_C_COMPILER   gcc)
    set(CMAKE_CXX_COMPILER g++)
# endif()

if ("${PROJECT_LANGUAGE}" STREQUAL "C")
    project("${PROJECT_NAME}" C)
elseif ("${PROJECT_LANGUAGE}" STREQUAL "C++")
    project("${PROJECT_NAME}" C CXX)
else()
    message(FATAL_ERROR "Unknown PROJECT_LANGUAGE \"${PROJECT_LANGUAGE}\", please set PROJECT_LANGUAGE to \"C\" or \"C++\"")
endif()


# --- Includes and settings ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress



# === Our project ===
add_executable(${PROJECT_NAME})

add_subdirectory(src)


# --- Project properties ---
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/../_deps)



# === Dependencies ===
if (${USE_RAYLIB} STREQUAL "TRUE")
    # https://github.com/raysan5/raylib/wiki/CMake-Build-Options
    # set(INCLUDE_EVERYTHING ON CACHE BOOL "" FORCE)
    set(SUPPORT_GESTURES_SYSTEM OFF CACHE BOOL "" FORCE)
    set(SUPPORT_MOUSE_GESTURES  OFF CACHE BOOL "" FORCE)
    # set(SUPPORT_GIF_RECORDING   OFF CACHE BOOL "" FORCE)
    set(SUPPORT_SAVE_PNG        OFF CACHE BOOL "" FORCE)
    set(SUPPORT_SAVE_BMP        OFF CACHE BOOL "" FORCE)

    ## File formats
    # Images
    set(SUPPORT_FILEFORMAT_GIF  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_QOI  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_DDS  OFF CACHE BOOL "" FORCE)
    # Fonts
    set(SUPPORT_FILEFORMAT_FNT  OFF CACHE BOOL "" FORCE)
    # Other
    set(SUPPORT_FILEFORMAT_OBJ  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_MTL  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_IQM  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_VOX  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_M3D  OFF CACHE BOOL "" FORCE)
    # Sound
    set(SUPPORT_FILEFORMAT_QOA  OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_XM   OFF CACHE BOOL "" FORCE)
    set(SUPPORT_FILEFORMAT_MOD  OFF CACHE BOOL "" FORCE)

    if(LINUX)
        set(GLFW_BUILD_X11     ON CACHE BOOL "" FORCE)
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
        URL_HASH SHA256=df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
    )

    FetchContent_MakeAvailable(raylib)

    set(raylib_VERBOSE TRUE)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

if (${USE_RAYGUI} STREQUAL "TRUE")
    # Fetch raygui if pkgconfig and raygui wasn't found
    find_package(PkgConfig)
    if (PkgConfig_FOUND)
        pkg_check_modules(raygui IMPORTED_TARGET raygui)
    endif()

    if (raygui_FOUND)
        set(RAYGUI_TARGET PkgConfig::raygui)
    else()
        # https://github.com/raysan5/raygui/releases
        FetchContent_Declare(
            raygui
            URL https://github.com/raysan5/raygui/archive/refs/tags/${RAYGUI_VERSION}.tar.gz
            URL_HASH SHA256=0f194c4a5e837c0930aca0b6315db45d00f76fa0052d841eea94598d390c39d6
        )
        FetchContent_MakeAvailable(raygui)

        add_library(raygui_headers INTERFACE)
        target_include_directories(raygui_headers SYSTEM INTERFACE
            "${raygui_SOURCE_DIR}/src"
        )
        set(RAYGUI_TARGET raygui_headers)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYGUI_TARGET})
endif()


# === Copying assets ===
# add_custom_command(
#     TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             "${CMAKE_SOURCE_DIR}/${ASSETS_DIR}"
#             "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ASSETS_DIR}"
# )


# === OS and BUILD_TYPE specific configurations ===
target_compile_options(${PROJECT_NAME}
    PRIVATE
        # -Wall            # Standard helpful warnings
        # -Wextra          # Extra warnings
        # -Wpedantic       # Enforces strict ISO compliance
        # -Wconversion     # Warns about implicit conversions that may alter values
        # -Wshadow         # Warns if a local variable hides another variable
        # -Wuninitialized  # Warns if you use a variable before setting a value
        # -Wformat=2       # Warns about format string issues in functions like printf/TraceLog
        # -Werror          # Treat warning as errors
)

if (WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -static)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            opengl32
            gdi32
            winmm
            comdlg32
            ole32
    )
elseif (APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreAudio"
            "-framework CoreVideo"
            "-framework QuartzCore"
    )
else()
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            GL
            m
            pthread
            dl
            rt
            X11
    )
endif()

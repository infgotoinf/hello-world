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

if ("${PROJECT_LANGUAGE}" STREQUAL "C")
    project("${PROJECT_NAME}" C)
elseif ("${PROJECT_LANGUAGE}" STREQUAL "C++")
    project("${PROJECT_NAME}" C CXX)
else()
    message(FATAL_ERROR "Unknown PROJECT_LANGUAGE \"${PROJECT_LANGUAGE}\", please set PROJECT_LANGUAGE to \"C\" or \"C++\"")
endif()

if (NOT ("${PLATFORM}" STREQUAL "Web" OR "${PLATFORM}" STREQUAL "Desktop"))
    message(FATAL_ERROR "Unknown PLATFORM \"${PLATFORM}\", please set PLATFORM to \"Web\" or \"Desktop\"")
endif()


# --- Includes and settings ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE) # Show download progress


# --- OS and PLATFORM specific configuration ---
if ("${PLATFORM}" STREQUAL "Desktop")
    # APPLE and BSD to clang, cause this is how it's done in the official raylib template
    if (APPLE OR BSD)
        set(CMAKE_C_COMPILER   clang)
        set(CMAKE_CXX_COMPILER clang++)
    else()
        set(CMAKE_C_COMPILER   gcc)
        set(CMAKE_CXX_COMPILER g++)
    endif()
endif()



# === Our project ===
add_executable(${PROJECT_NAME})

add_subdirectory(src)


# --- Project properties ---
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/../../_deps)



# === Dependencies ===
if (${USE_RAYLIB} STREQUAL "TRUE")
    # Fetch raylib if it wasn't found
    # https://github.com/raysan5/raylib/releases
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
        URL_HASH SHA256=df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
        FIND_PACKAGE_ARGS
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
        target_include_directories(raygui_headers INTERFACE
            "${raygui_SOURCE_DIR}/src"
        )
        set(RAYGUI_TARGET raygui_headers)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE ${RAYGUI_TARGET})
endif()



# === Copying assets ===
if (NOT ("${PLATFORM}" STREQUAL "Web"))
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/${ASSETS_DIR}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ASSETS_DIR}"
    )
endif()


# === OS and PLATFORM specific configurations ===
if ("${PLATFORM}" STREQUAL "Web")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-sUSE_GLFW=3"
            "--preload-file ${CMAKE_SOURCE_DIR}/${ASSETS_DIR}@/${ASSETS_DIR}"
            "-sFORCE_FILESYSTEM=1"
            "-sMINIFY_HTML=0"
    )
    if ("${BUILD_TYPE}" STREQUAL "Debug")
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
                "-sASSERTIONS=2"
                "--profiling"
        )
    elseif ("${BUILD_TYPE}" STREQUAL "Release")
        target_link_libraries(${PROJECT_NAME}
            PRIVATE
                "--shell-file ${CMAKE_SOURCE_DIR}/resources/minshell.html"
        )
    endif()
elseif ("${PLATFORM}" STREQUAL "Desktop")
    if (WIN32)
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
endif()

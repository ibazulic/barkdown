cmake_minimum_required(VERSION 3.20)
project("Barkdown Viewer"
    VERSION 1.0.0
    LANGUAGES CXX
)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "In-source builds are forbidden. Create a build outside of the source tree.")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# put compiled binaries in predictable directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# main project executable
add_executable(barkdown)

# test executable
add_executable(barkdown-test)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# find cmark library
find_package(cmark REQUIRED)

# find SDL library
find_package(SDL2 REQUIRED)
if(TARGET SDL2::SDL2)
    target_link_libraries(barkdown
        PRIVATE
            SDL2::SDL2        
    )
else()
    target_link_libraries(barkdown
        PRIVATE
            ${SDL2_LIBRARIES}
    )   
    target_include_directories(barkdown 
        PRIVATE
            ${SDL2_INCLUDE_DIRS}
    )
endif()

# find OpenGL library
find_package(OpenGL REQUIRED)

# find Curl library
find_package(CURL REQUIRED)

# Find threads package
find_package(Threads REQUIRED)

# Add source files to the main target
target_sources(barkdown
    PRIVATE
        src/main.cpp
        src/render.cpp
        src/markdown.cpp
        src/window.cpp
        src/file_operations.cpp
        src/image_loader.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
        third_party/imgui/backends/imgui_impl_sdl2.cpp
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        # third_party/imgui/imgui_demo.cpp
)
target_link_libraries(barkdown
    PRIVATE
        cmark
        OpenGL::GL
        Threads::Threads
        CURL::libcurl
)

# Add sources to the test target
target_sources(barkdown-test 
    PRIVATE
        src/markdown.cpp
        tests/test_parser.cpp
        tests/test_path_parsing.cpp
        src/image_loader.cpp
)
target_link_libraries(barkdown-test
    PRIVATE
        gtest_main
        cmark
        Threads::Threads
        CURL::libcurl
        OpenGL::GL
)

# include directories
target_include_directories(barkdown
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/portable-file-dialogs
)

# include directories
target_include_directories(barkdown-test
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src

)

# format all source depending on a specific style
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/include/*.hpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES} --verbose
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

# Add compile definition for the test file
target_compile_definitions(barkdown-test
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests"
)


# enable tests
enable_testing()
include(GoogleTest)
gtest_discover_tests(barkdown-test)

if(MSVC)
    target_compile_options(barkdown PRIVATE /W4 /Wx /permissive-)
    target_compile_options(barkdown-test PRIVATE /W4 /Wx /permissive-)
else()
    target_compile_options(barkdown PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-error=unused-variable -Wno-unused-but-set-variable)
    target_compile_options(barkdown-test PRIVATE -Wall -Wextra -Wpedantic -Wno-error=unused-variable -Wno-unused-but-set-variable)
endif()

# install compiled binary to /usr/local/bin
install(TARGETS barkdown DESTINATION bin)

# package metadata
set(CPACK_PACKAGE_NAME "barkdown")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_CONTACT "Ivan Bazulic <ibazulic@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast, multithreaded, GPU-accelerated Markdown viewer")

# package kind
set(CPACK_GENERATOR "RPM;DEB")

# package requirements
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libcurl4, libcmark0.30.2, libgl1")

set(CPACK_RPM_PACKAGE_REQUIRES "SDL2, libcurl, cmark-lib, mesa-libGL")
include(CPack)

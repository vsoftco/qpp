cmake_minimum_required(VERSION 3.12)
project(stress_tests VERSION 2.6 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#### Guard against in-source builds (snippet from Eigen's CMakeLists.txt)
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
endif ()

add_library(libqpp INTERFACE)
target_include_directories(libqpp INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>)

#### Dependencies, do not modify unless you know what you're doing
include(../cmake/qpp_dependencies.cmake)

#### Enable all warnings for GNU gcc and Clang/AppleClang
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID}
        STREQUAL "GNU")
    add_compile_options("-pedantic" "-Wall" "-Wextra" "-Weffc++")
    if (${SANITIZE})
        if (NOT (${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang"))
            list(APPEND SANITIZE_OPTIONS -fsanitize=undefined)
            add_compile_options("${SANITIZE_OPTIONS}")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_OPTIONS}")
        endif ()
    endif ()
endif ()

#### Source file(s) to be compiled, modify as needed
aux_source_directory(${CMAKE_SOURCE_DIR}/src STRESS_TESTS)

#### Build all stress tests in ${STRESS_TESTS}
foreach (FILE ${STRESS_TESTS})
    get_filename_component(TARGET_NAME ${FILE} NAME_WE)
    add_executable(${TARGET_NAME} ${FILE})
    target_link_libraries(${TARGET_NAME} PUBLIC ${QPP_LINK_DEPS} libqpp)
endforeach ()

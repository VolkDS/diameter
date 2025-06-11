cmake_minimum_required(VERSION 3.14)

project(diameter LANGUAGES CXX)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_INCLUDE_PATH})

### Options
option(DIAMETER_BUILD_TESTS "Build tests" OFF)
option(DIAMETER_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(DIAMETER_BUILD_EXAMPLES "Build examples" OFF)
option(DIAMETER_BUILD_DOCS "Build documentation" OFF)
option(DIAMETER_HEADER_ONLY "Build header only" OFF)
option(DIAMETER_ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(DIAMETER_ENABLE_CPPCHECK "Enable static analysis" OFF)
option(DIAMETER_USE_SUBMODULES "Use submodules for external libraries" OFF)

### Build type
set(CMAKE_CONFIGURATION_TYPES Debug Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("Current configuration type: ${CMAKE_BUILD_TYPE}")

include(CompileSettings)

### CppCheck
if (DIAMETER_ENABLE_CPPCHECK)
    find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
    if (CMAKE_CXX_CPPCHECK)
        set(CPP_CHECK_CMD ${CMAKE_CXX_CPPCHECK} --version)
        execute_process(COMMAND ${CPP_CHECK_CMD}
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          RESULT_VARIABLE CPP_CHECK_RESULT
          OUTPUT_VARIABLE CPP_CHECK_VERSION
          ERROR_VARIABLE CPP_CHECK_ERROR)

        # Check if version could be extracted
        if(CPP_CHECK_RESULT EQUAL 0)
            message("Found cppcheck version: ${CPP_CHECK_VERSION}")
            list(
                APPEND CMAKE_CXX_CPPCHECK
                    "--template=gcc"
                    "--enable=warning,style,performance,portability"
                    "--quiet"
                    "--inconclusive"
                    "--force"
                    "--inline-suppr"
                    "--suppressions-list=${CMAKE_SOURCE_DIR}/CppCheckSuppressions.txt"
                    "--language=c++"
                    "--platform=unix64"
                    "--std=c++17"
            )
        endif()
    else()
        message(FATAL_ERROR "Not found cppcheck")
    endif()
endif()

### External targets
if(is_top_level)
    add_subdirectory(externals)
endif()

### Check mandatory dependenties
if(NOT TARGET netpacker::netpacker)
  message(FATAL_ERROR "Not found target netpacker::netpacker")
endif()

### Library target
if (DIAMETER_HEADER_ONLY)
    add_library(${PROJECT_NAME} INTERFACE)
    add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    target_include_directories(${PROJECT_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    )
    target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
    target_link_libraries(${PROJECT_NAME} INTERFACE netpacker::netpacker)
else()
    file(GLOB_RECURSE DIAMETER_SRC src/*.cpp)
    source_group ("Source Files" FILES ${DIAMETER_SRC})

    add_library(${PROJECT_NAME} STATIC ${DIAMETER_SRC})
    add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    )

    default_cxx_compile_settings_default(${PROJECT_NAME})
    default_cxx_compile_settings_warnings(${PROJECT_NAME})

    target_link_libraries(${PROJECT_NAME} PUBLIC netpacker::netpacker)
endif()

### Other targets
if(DIAMETER_BUILD_TESTS)
    message(STATUS "Building of tests is enabled")
    enable_testing()
    add_subdirectory(tests)
endif()

if(DIAMETER_BUILD_EXAMPLES)
    message(STATUS "Building of examples is enabled")
    add_subdirectory(examples)
endif()

if(DIAMETER_BUILD_DOCS)
    message(STATUS "Building of docs is enabled")
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)

    doxygen_add_docs(docs
        ${DOXYGEN_INPUT}
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        COMMENT "Generating documentation"
    )
endif()

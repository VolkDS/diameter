find_package(Boost 1.69 REQUIRED COMPONENTS unit_test_framework)

file(GLOB TEST_SOURCES 
    "unit/*/*.cpp"
    "helpers/*.cpp"
)

add_executable(tests
    test_main.cpp
    ${TEST_SOURCES}
)

target_link_libraries(tests PRIVATE
    diameter::diameter
    Boost::unit_test_framework
)

default_cxx_compile_settings_default(tests)
default_cxx_compile_settings_warnings(tests)

if (DIAMETER_ENABLE_COVERAGE)
    target_compile_options(tests PRIVATE
        --coverage
        -fprofile-arcs
        -ftest-coverage
    )
    target_link_options(tests PRIVATE
        --coverage
    )
endif()

add_test(NAME diameter_tests COMMAND tests --log_level=message)

if (DIAMETER_ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov REQUIRED)
    find_program(GENHTML_PATH genhtml REQUIRED)

    add_custom_target(coverage
        # Очистка предыдущих данных
        COMMAND ${LCOV_PATH} --directory . --zerocounters
        # Запуск тестов
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        # Сбор данных
        COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
        # Удаление системных файлов из отчета
        COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/externals/*' --output-file coverage.info
        # Генерация HTML
        COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_report
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage_report/index.html"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS tests
    )
endif()

if(DIAMETER_BUILD_BENCHMARKS)
    message(STATUS "Building of benchmarks is enabled")
    add_subdirectory(benchmarks)
endif()

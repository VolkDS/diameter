file(GLOB BENCHMARK_SOURCES 
    "*.cpp"
)

add_executable(benchmarks
    ${BENCHMARK_SOURCES}
)

target_link_libraries(benchmarks PRIVATE
    diameter::diameter
    benchmark::benchmark
)

add_test(NAME diameter_benchmarks COMMAND benchmarks)

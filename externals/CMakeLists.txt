if (DIAMETER_USE_SUBMODULES)
  add_subdirectory(netpacker)
else()
  find_package(Netpacker)
  set_target_properties(netpacker::netpacker PROPERTIES IMPORTED_GLOBAL TRUE)
endif()

if(DIAMETER_BUILD_BENCHMARKS)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Switch off tests" FORCE)
  set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "Build Release candidates with -Werror." FORCE)
  add_subdirectory(google-benchmark)
endif()

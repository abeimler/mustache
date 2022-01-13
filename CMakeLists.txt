cmake_minimum_required(VERSION 3.10)
project("")
set(CMAKE_CXX_STANDARD 17)

if(MUSTACHE_SANITIZE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

add_subdirectory(third_party)
add_subdirectory(src)

if (MUSTACHE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (MUSTACHE_BUILD_BENCHMARK)
    add_subdirectory(bench)
endif()

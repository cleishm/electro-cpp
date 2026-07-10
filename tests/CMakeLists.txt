include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(electro_tests
    electro_test.cpp
    decibel_test.cpp
)

target_link_libraries(electro_tests PRIVATE
    electro::electro
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(electro_tests)

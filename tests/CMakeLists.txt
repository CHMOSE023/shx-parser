include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Prevent overriding the parent project's compiler/linker settings (Windows)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(SHXTests 
    TestArc.cpp
    TestHeadeParser.cpp
    TestPoint.cpp
)

target_link_libraries(SHXTests
    PRIVATE
        SHXParser
        GTest::gtest_main
)

target_compile_features(SHXTests PRIVATE cxx_std_17)

include(GoogleTest)
gtest_discover_tests(SHXTests)

add_executable(SHXExample example.cpp)

target_link_libraries(SHXExample PRIVATE SHXParser)
target_compile_features(SHXExample PRIVATE cxx_std_17)

# Inject the absolute path to the data/ directory so the example can find
# SHX files regardless of the working directory at runtime.
target_compile_definitions(SHXExample PRIVATE
    SHX_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

if(MSVC)
    target_compile_options(SHXExample PRIVATE /W4)
    target_compile_definitions(SHXExample PRIVATE _USE_MATH_DEFINES)
else()
    target_compile_options(SHXExample PRIVATE -Wall -Wextra)
endif()

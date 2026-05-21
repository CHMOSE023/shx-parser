if(SHX_BUILD_SHARED)
    add_library(SHXParser SHARED
        Arc.cpp
        Shape.cpp
        FileReader.cpp
        HeaderParser.cpp
        ContentParser.cpp
        ShapeParser.cpp
        Font.cpp
    )
else()
    add_library(SHXParser STATIC
        Arc.cpp
        Shape.cpp
        FileReader.cpp
        HeaderParser.cpp
        ContentParser.cpp
        ShapeParser.cpp
        Font.cpp
    )
endif()

target_include_directories(SHXParser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(SHXParser PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(SHXParser PRIVATE /W4)
    # MSVC does not define M_PI by default; _USE_MATH_DEFINES enables it.
    target_compile_definitions(SHXParser PUBLIC _USE_MATH_DEFINES)
else()
    target_compile_options(SHXParser PRIVATE -Wall -Wextra -Wpedantic)
endif()

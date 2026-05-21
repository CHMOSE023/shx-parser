cmake_minimum_required(VERSION 3.16)
project(shx_parser VERSION 1.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SHX_BUILD_TESTS    "Build unit tests" ON) 
option(SHX_BUILD_EXAMPLES "Build examples" ON)

add_subdirectory(src)

if(SHX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SHX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install rules
include(GNUInstallDirs)
install(TARGETS SHXParser
    EXPORT shx_parserTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

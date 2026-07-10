cmake_minimum_required(VERSION 3.14)

# ESP-IDF component support (must be before project() for script mode compatibility)
if(ESP_PLATFORM)
    idf_component_register(INCLUDE_DIRS "include")
    return()
endif()

project(electro
    VERSION 0.3.0
    DESCRIPTION "Type-safe electrical units library modeled after std::chrono"
    HOMEPAGE_URL "https://github.com/cleishm/electro-cpp"
    LANGUAGES CXX
)

option(ELECTRO_BUILD_TESTS "Build tests" ${electro_IS_TOP_LEVEL})

set(CMAKE_CXX_EXTENSIONS OFF)

# Main library target (header-only)
add_library(electro INTERFACE)
add_library(electro::electro ALIAS electro)

target_include_directories(electro INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(electro INTERFACE cxx_std_20)

# Formatting (only when this is the top-level project)
if(electro_IS_TOP_LEVEL)
    find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

    if(CLANG_FORMAT_EXECUTABLE)
        # Get all source files
        file(GLOB_RECURSE ALL_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
            ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
        )

        # Target to check formatting
        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${ALL_SOURCE_FILES}
            COMMENT "Checking code formatting"
            VERBATIM
        )

        # Target to fix formatting
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
            COMMENT "Applying code formatting"
            VERBATIM
        )

        message(STATUS "clang-format found: format targets available")
    else()
        message(WARNING "clang-format not found: format targets unavailable")
    endif()
endif()

# Tests
if(ELECTRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
option(ELECTRO_BUILD_DOCS "Build documentation" OFF)

if(ELECTRO_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        add_custom_target(docs
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUTPUT_DIRECTORY}
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        message(STATUS "Doxygen found: documentation can be built with 'make docs'")
    else()
        message(WARNING "Doxygen not found: documentation cannot be built")
    endif()
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS electro
    EXPORT electroTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT electroTargets
    FILE electroTargets.cmake
    NAMESPACE electro::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/electro
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/electroConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/electroConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/electro
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/electroConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/electroConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/electroConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/electro
)

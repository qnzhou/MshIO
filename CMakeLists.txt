# ============================================================================
# This file will generate the cmake target `MshIO::MshIO`.
#
# To add MshIO as a dependency:
#
#   add_subdirectory(MshIO)
#   target_link_libraries(your_target mshio::mshio)
#
# ============================================================================

cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 14)

project(MshIO)

option(MSHIO_BUILD_TESTS "Build unit tests" OFF)
option(MSHIO_BUILD_EXAMPLES "Build examples" OFF)
option(MSHIO_EXT_NANOSPLINE "Enable nanospline extension" OFF)

if (NOT SKBUILD)
    option(MSHIO_PYTHON "Build python binding" OFF)
else()
    set(MSHIO_PYTHON ON CACHE BOOL "Build python binding" FORCE)
endif()

include(FetchContent)

file(GLOB INC_FILES "${PROJECT_SOURCE_DIR}/include/mshio/*.h")
file(GLOB SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(mshio STATIC ${SRC_FILES})
if(NOT MSVC)
    target_compile_options(mshio PRIVATE -Wconversion -Wall -Werror
        -Wno-deprecated-declarations)
endif()
target_include_directories(mshio PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>")
set_property(TARGET mshio PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(mshio::mshio ALIAS mshio)


if (MSHIO_EXT_NANOSPLINE)
    target_compile_definitions(mshio PUBLIC -DMSHIO_EXT_NANOSPLINE)
endif()


if (MSHIO_PYTHON)
    include(cmake/nanobind.cmake)
    set(PY_SRC_FILE "${PROJECT_SOURCE_DIR}/python/pymshio.cpp")
    nanobind_add_module(pymshio NB_STATIC ${PY_SRC_FILE})
    if (NOT MSVC)
        target_compile_options(pymshio PRIVATE -Wno-deprecated-declarations)
    endif()
    target_link_libraries(pymshio PUBLIC mshio::mshio)
    install(TARGETS pymshio LIBRARY DESTINATION .)
    add_library(mshio::pymshio ALIAS pymshio)

    include(cmake/sanitizer-cmake.cmake)
    if (SANITIZE_ADDRESS OR
            SANITIZE_LINK_STATIC OR
            SANITIZE_MEMORY OR
            SANITIZE_THREAD OR
            SANITIZE_UNDEFINED)
        add_sanitizers(pymshio)
        target_link_options(pymshio PRIVATE "-shared-libasan")
    endif()
endif()


if (MSHIO_BUILD_EXAMPLES)
    add_executable(msh_inspect ${PROJECT_SOURCE_DIR}/examples/msh_inspect.cpp)
    target_link_libraries(msh_inspect PRIVATE mshio::mshio)
    target_compile_options(msh_inspect PRIVATE -Wno-deprecated-declarations)
endif()


if (MSHIO_BUILD_TESTS)
    include(CTest)
    enable_testing()
    include(cmake/Catch2.cmake)
    include(cmake/sanitizer-cmake.cmake)

    file(GLOB TEST_FILES "${PROJECT_SOURCE_DIR}/tests/*.cpp")
    add_executable(test_MshIO ${TEST_FILES})
    target_link_libraries(test_MshIO mshio::mshio Catch2::Catch2)
    target_compile_definitions(test_MshIO PRIVATE
        MSHIO_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
    catch_discover_tests(test_MshIO)

    if(NOT MSVC)
        target_compile_options(test_MshIO PRIVATE -Wconversion -Wall -Werror
            -Wno-deprecated-declarations)
    else()
        target_compile_options(test_MshIO PRIVATE "/MP")
    endif()

    if (SANITIZE_ADDRESS OR
            SANITIZE_LINK_STATIC OR
            SANITIZE_MEMORY OR
            SANITIZE_THREAD OR
            SANITIZE_UNDEFINED)
        add_sanitizers(test_MshIO)
    endif()
endif()


if (NOT MSHIO_PYTHON)
    include(GNUInstallDirs)
    install(TARGETS mshio
            EXPORT mshio_target
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/ DESTINATION include
            FILES_MATCHING PATTERN "*.h")
    install(EXPORT mshio_target DESTINATION cmake)
endif()

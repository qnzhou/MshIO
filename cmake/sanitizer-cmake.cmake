if(COMMAND add_sanitizers)
    return()
endif()

message(STATUS "Third-party (external): creating method 'add_sanitizers'")

include(CPM)
CPMAddPackage(
    NAME sanitizer
    GITHUB_REPOSITORY arsenm/sanitizers-cmake
    GIT_TAG 0573e2ea8651b9bb3083f193c41eb086497cc80a
    DOWNLOAD_ONLY ON
)

block()
set(CMAKE_MODULE_PATH "${sanitizer_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers REQUIRED)
endblock()

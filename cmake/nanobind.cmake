if (COMMAND nanobind_add_module)
    return()
endif()

include(cmake/python.cmake)
FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v0.1.0
    GIT_SHALLOW OFF)

FetchContent_Populate(nanobind)
find_package(nanobind PATHS ${nanobind_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

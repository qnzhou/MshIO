if(COMMAND nanobind_add_module)
    return()
endif()

message(STATUS "Third-party (external): creating target 'nanobind::nanobind'")

include(CPM)
CPMAddPackage(
    NAME nanobind
    GITHUB_REPOSITORY wjakob/nanobind
    GIT_TAG v2.9.2
    DOWNLOAD_ONLY ON
)

include(python)
set(NB_SHARED OFF CACHE INTERNAL "")

# Set this until https://github.com/wjakob/nanobind/issues/1123 is resolved.
set(NB_PY_PATH "${Python_EXECUTABLE}" CACHE INTERNAL "Path to Python executable for nanobind")

# Note that we do not use FetchContent_MakeAvailable here because nanobind's cmake changes
# CMAKE_CXX_FLAGS and attempts to refind python, which can leads to cmake error.
find_package(nanobind PATHS ${nanobind_SOURCE_DIR}/cmake NO_DEFAULT_PATH)

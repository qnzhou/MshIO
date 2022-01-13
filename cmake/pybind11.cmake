if (TARGET pybind11::pybind11)
    return()
endif()

FetchContent_Declare(
    PyBind11
    GIT_REPOSITORY git@github.com:pybind/pybind11.git
    GIT_TAG v2.9.0
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(pybind11)

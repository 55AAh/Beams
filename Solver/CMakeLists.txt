if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    set(BUILD_SHARED_LIBS TRUE)
endif ()


add_library(Solver SHARED
    formulae.h
    Solver.cpp
)
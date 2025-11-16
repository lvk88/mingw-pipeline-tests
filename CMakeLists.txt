cmake_minimum_required(VERSION 3.28)

project(mingw-pipeline-test)

include(FetchContent)

# Declare and get dependencies
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG b3fb4b9feafcd8d91c5cb510a4775143fdbef02f # v3.11.0
)

FetchContent_MakeAvailable(catch2)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8 # v3.0.1
)

# when including pybind11, it will trigger a find_package(Python) search. This
# in turn will internally try to call a find_program(...) . If we are cross
# compiling, we need to tell CMake to not try to look in the
# CMAKE_FIND_ROOT_PATHS of the host system.
if(CMAKE_CROSSCOMPILING)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
endif(CMAKE_CROSSCOMPILING)

FetchContent_MakeAvailable(pybind11)

if(CMAKE_CROSSCOMPILING)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
endif(CMAKE_CROSSCOMPILING)

# Add a library  
add_library(foo lib.cpp)
target_include_directories(foo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(foo PUBLIC cxx_std_20)
install(TARGETS foo)

# Test the library
add_executable(tests test/test.cpp)
target_link_libraries(tests PRIVATE foo Catch2::Catch2WithMain)
install(TARGETS tests)

# Add python module that uses the library
pybind11_add_module(pylib pylib.cpp)
target_link_libraries(pylib PUBLIC foo)
install(TARGETS pylib LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR})

# Make sure that we copy the mingw dll-s to our output folder when installing
if(MINGW)
    foreach(_MINGW_DLL_DEP IN ITEMS libgcc_s_seh-1.dll libgomp-1.dll libstdc++-6.dll libwinpthread-1.dll)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER}
            -print-file-name=${_MINGW_DLL_DEP}
            OUTPUT_VARIABLE _MINGW_DLL_DEP_LOCATION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(EXISTS ${_MINGW_DLL_DEP_LOCATION})
            INSTALL(PROGRAMS ${_MINGW_DLL_DEP_LOCATION} TYPE BIN)
        endif(EXISTS ${_MINGW_DLL_DEP_LOCATION})
    endforeach()
endif(MINGW)

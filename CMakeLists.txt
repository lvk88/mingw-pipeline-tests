cmake_minimum_required(VERSION 3.28)

project(mingw-pipeline-test)

include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG b3fb4b9feafcd8d91c5cb510a4775143fdbef02f # v3.11.0
)

FetchContent_MakeAvailable(catch2)

add_library(foo lib.cpp)
add_executable(tests test.cpp)

target_link_libraries(tests PRIVATE foo Catch2::Catch2WithMain)

install(TARGETS tests)

if(MINGW)
    foreach(_MINGW_DLL_DEP IN ITEMS libgcc_s_seh-1.dll libgomp-1.dll libstdc++-6.dll libwinpthread-1.dll)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER}
            -print-file-name=${_MINGW_DLL_DEP}
            OUTPUT_VARIABLE _MINGW_DLL_DEP_LOCATION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(EXISTS ${_MINGW_DLL_DEP_LOCATION})
            #MESSAGE(STATUS "Adding install rule for ${_MINGW_DLL_DEP_LOCATION}")
            INSTALL(PROGRAMS ${_MINGW_DLL_DEP_LOCATION} TYPE BIN)
        endif(EXISTS ${_MINGW_DLL_DEP_LOCATION})
    endforeach()
endif(MINGW)

# pybind11 with MinGW on Linux

| GitLab |[![gitlab-status](https://gitlab.com/laszlokudela/mingw-pipeline-tests/badges/main/pipeline.svg)](https://gitlab.com/laszlokudela/mingw-pipeline-tests/-/pipelines?page=1&scope=all&ref=main) |
|--------|---------|
| GitHub |[![build-and-test](https://github.com/lvk88/mingw-pipeline-tests/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/lvk88/mingw-pipeline-tests/actions/workflows/build-and-test.yml) |

This repository contains an example for cross-compiling a Python extension module using pybind11 for Windows on a Debian machine.

## Quick start

The steps below build the extension module in a Debian docker container.

```
git clone https://github.com/lvk88/mingw-pipeline-tests.git
cd mingw-pipeline-tests
docker run --rm -it -v $PWD:/work debian:trixie bash
cd /work
apt update
apt install -y --no-install-recommends git cmake ninja-build g++-mingw-w64-x86-64 gcc-mingw-w64-x86-64 wget curl mono-devel
wget https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
mono nuget.exe install python -Version 3.13.0
cmake --preset mingw-release
cmake --build --preset mingw-release --target install
exit
```

The extension module will be located at:

```
mingw-pipeline-tests/build/mingw-release/install/pylib
```

The module contains a simple test. You can run the tests **in Windows in a PowerShell** instance:

```
cd mingw-pipeline-tests
$Env:PATH = "$PWD/python.3.13.0/tools;$Env:PATH"
cd build/mingw-release/install
python -m venv venv
pip install numpy pytest
pytest .
```

There are two CI setups that perform these steps:

* [GitHub workflow building on Debian and running the tests on Windows](https://github.com/lvk88/mingw-pipeline-tests/actions/workflows/build-and-test.yml)
* [GitLab pipeline building on Debian and running the tests on Windows](https://gitlab.com/laszlokudela/mingw-pipeline-tests/-/pipelines)

## The nitty-gritty details

### Specifying the compilers and CMAKE_SYSTEM_NAME

The easiest part when cross-compiling is to let CMake know where is our compiler, and the type of the system we are cross-compiling for:

```bash
cmake \
-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
-DCMAKE_SYSTEM_NAME=Windows \
-S . -B build/
```

### CMake and find root path

The non-trivial part when cross compiling on a Linux host for a Windows target is that want CMake to find and use *some but not all* of the dependencies and tools from the Linux host. For example, we want CMake to find the build system (`Ninja` or `make`) from the host, and at the same time we want CMake to find includes and libraries from our target.

The relevant commands that CMake uses for the search are [find_program](https://cmake.org/cmake/help/latest/command/find_program.html), [find_library](https://cmake.org/cmake/help/latest/command/find_library.html#command:find_library), [find_file](https://cmake.org/cmake/help/latest/command/find_file.html#command:find_file), and [find_path](https://cmake.org/cmake/help/latest/command/find_path.html#command:find_path). 

In the case of cross-compiling, we have our dependencies under custom paths, and we need to tell CMake to include these paths in its search order when looking for files/programs/libraries. We do this by extending [CMAKE_FIND_ROOT_PATH](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH.html):

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
+-DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
 -S . -B build/
```

We also want to explicitly avoid that CMake looks for libraries and includes on the host system, and `ONLY` looks for them in the `CMAKE_FIND_ROOT_PATH` we just specified:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
+-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
+-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -S . -B build/
```

See documentation for [CMAKE_FIND_ROOT_PATH_MODE_INCLUDE](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_INCLUDE.html) and [CMAKE_FIND_ROOT_PATH_MODE_LIBRARY](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_LIBRARY.html). Shortly the value `ONLY` means that we only want CMake to look in `CMAKE_FIND_ROOT_PATH`.

Note that it easily leads to an error to specify `-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=ONLY`. When doing this, we instruct CMake to ignore the paths on the host system when looking for `PROGRAM`-s, including the build system. CMake would not find `Ninja` or `make` and exit with an error. In fact, what we want is to tell cmake the opposite: when looking for `PROGRAM`-s we `NEVER` want CMake to look in the root environment of our cross-setup:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
+-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -S . -B build/
```

This command will ensure that:

* external libraries are searched for only in `CMAKE_FIND_ROOT_PATH`
* programs are searched for only in the paths of the host system.

So far so good, now let`s get to the complications.

### Getting a Windows Python

If we want to link against Windows' Python, we need to get it from somewhere. 
The official Python documentation [recommends that we get it from NuGet](https://docs.python.org/3/using/windows.html#the-nuget-org-packages).

NuGet is a `dotnet` application, therefore we need to install dotnet first. On Debian, we can use mono:

```
sudo apt install mono-devel
```

And we download NuGet:

```
wget --no-check-certificate https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
```

And finally we can tell NuGet to download Python. This example uses Python 3.13.0:

```
mono nuget.exe install python -Version 3.13.0
```

This will download and install Python 3.13.0 in `$PWD/python.3.13.0`:

```diff
 .
 ├── CMakeLists.txt
 ├── CMakePresets.json
 ├── README.md
 ├── lib.cpp
 ├── lib.hpp
 ├── nuget.exe
 ├── pylib.cpp
+├── python.3.13.0
+│   ├── build
+│   │   └── native
+│   ├── images
+│   │   └── python.png
+│   ├── python.3.13.0.nupkg
+│   └── tools
+│       ├── DLLs
+│       ├── LICENSE.txt
+│       ├── Lib
+│       ├── include
+│       ├── libs
+│       ├── python.exe
+│       ├── python3.dll
+│       ├── python313.dll
+│       ├── pythonw.exe
+│       ├── vcruntime140.dll
+│       └── vcruntime140_1.dll
```

Now that we have python, we add the path to it `CMAKE_FIND_ROOT_PATH`:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
+-DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32;$PWD/python.3.13.0/tools \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -S . -B build/
```

Let's include pybind11 in our cmake project.

```
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8 # v3.0.1
)
FetchContent_MakeAvailable(pybind11)
```

Running CMake will fail now with this error message: 

```
Could NOT find Python (missing: Python_LIBRARIES Python_INCLUDE_DIRS
Development.Module) (found suitable version "3.12.3", minimum required is
```

The problem is that including `pybind11` will eventually call `find_package(Python)` (see [here](https://github.com/pybind/pybind11/blob/f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8/tools/pybind11NewTools.cmake#L54)), which will execute CMake's built in [find_python](https://github.com/Kitware/CMake/blob/b196a40b0dd1c94ad2fa3cacfc445c8b60d71467/Modules/FindPython/Support.cmake), which gets confused about our cross compiling setup. It finds some parts of Python from the host (Linux), but it also understands that it needs to find Python from the target (Windows), and eventually it gives up. Therefore, we need to tell CMake to ONLY look in our `CMAKE_ROOT_PATH` when looking for python. To do that, before including pybind11, we set `CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=ONLY`, and set it back to `NEVER` after `pybind11` has initialized itself:

```diff
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8 # v3.0.1
)
+ if(CMAKE_CROSSCOMPILING)
+  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
+ endif(CMAKE_CROSSCOMPILING)

FetchContent_MakeAvailable(pybind11)

+ if(CMAKE_CROSSCOMPILING)
+   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
+ endif(CMAKE_CROSSCOMPILING)
```

Running our cmake command again still results in failure. So we explicitly tell CMake where to look for the Python include dirs and the python library:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH="/usr/x86_64-w64-mingw32;$PWD/python.3.13.0" \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
+-DPython_INCLUDE_DIR=$PWD/python.3.13.0/tools/include \
+-DPython_LIBRARY=$$PWD/python.3.13.0/tools/python313.dll \
 -S . -B build/
```

This command still fails, because pybind11 also wants to find the interpreter, which turn into a `find_program` call, which will also try to execute the found interpreter, and it is not going to work out, because we are on Linux and we can't run the found Python interpeter that is a Windows executable. Fortunately, we can tell `pybind11` to NOT look for the interpeter by using `PYBIND11_USE_CROSSCOMPILING=ON`.

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH="/usr/x86_64-w64-mingw32;$PWD/python.3.13.0" \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -DPython_INCLUDE_DIR=$PWD/python.3.13.0/tools/include \
 -DPython_LIBRARY=$PWD/python.3.13.0/tools/python313.dll \
+-DPYBIND11_USE_CROSSCOMPILING=ON \
 -S . -B build/
```

This time CMake succesfully generates, which is great!

But we still have a problem:

```
CMake Warning at build/_deps/pybind11-src/tools/pybind11GuessPythonExtSuffix.cmake:33 (message):
  Python_SOABI is defined but empty.  You may want to set
  PYTHON_MODULE_EXT_SUFFIX explicitly.
```

Of course, now pybind11 doesn't know what kind of ABI we are building against, because it can't ask the interpeter. So we do as it suggests:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH="/usr/x86_64-w64-mingw32;$PWD/python.3.13.0" \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -DPython_INCLUDE_DIR=$PWD/python.3.13.0/tools/include \
 -DPython_LIBRARY=$PWD/python.3.13.0/tools/python313.dll \
 -DPYBIND11_USE_CROSSCOMPILING=ON \
+-DPYTHON_MODULE_EXT_SUFFIX=".cp313-win_amd64.pyd" \
 -S . -B build/
```

Now the warning message is gone, so we can try to build:

```
cmake --build build
```

This time the build will fail:

```
/usr/bin/x86_64-w64-mingw32-strip: '/home/[redacted]/dev/mingw-pipeline-tests/build/pylib': No such file
gmake[2]: *** [CMakeFiles/pylib.dir/build.make:103: pylib] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:193: CMakeFiles/pylib.dir/all] Error 2
gmake: *** [Makefile:136: all] Error 2
```

Even though we silenced the warning with `-DPYTHON_MODULE_EXT_SUFFIX="..."`, at the time we get to the [strip](https://github.com/pybind/pybind11/blob/f5fbe867d2d26e4a0a9177a51f6e568868ad3dc8/tools/pybind11Common.cmake#L445) step, CMake forgets about it. To get around this issue, we set yet another CMake variable:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH="/usr/x86_64-w64-mingw32;$PWD/python.3.13.0" \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -DPython_INCLUDE_DIR=$PWD/python.3.13.0/tools/include \
 -DPython_LIBRARY=$PWD/python.3.13.0/tools/python313.dll \
 -DPYBIND11_USE_CROSSCOMPILING=ON \
 -DPYTHON_MODULE_EXT_SUFFIX=".cp313-win_amd64.pyd" \
+-DPYTHON_MODULE_EXTENSION=".cp313-win_amd64.pyd" \
 -S . -B build/
```

Now building will work:

```
cmake --build build/
```

## Installing the right DLL-s to the right location

If we copy `build/pylib.cp313-win_amd64.pyd` over to a Windows machine and try to import it, Python will complain about missing dll-s:

```
Python 3.13.5 (tags/v3.13.5:6cb20a2, Jun 11 2025, 16:15:46) [MSC v.1943 64 bit (AMD64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>> import pylib
Traceback (most recent call last):
  File "<python-input-0>", line 1, in <module>
    import pylib
ImportError: DLL load failed while importing pylib: The specified module could not be found.
>>>
```

The missing DLL-s that python is looking for are those from MinGW:
* `libgcc_s_seh-1.dll`
* `libstdc++-6.dll`
* `libwinpthread-1.dll`

These dll-s need to be in the same directory as our `pyd` module, otherwise importing the `pyd` module will fail with the above error. To have them automatically copied, we do the following:

* We add an `INSTALL` rule for `pylib` that will install it into a `pylib` folder
* In our CMakeLists, we ask the compiler for the paths where these dll-s are located
* We introduce an `INSTALL` rule that copies these dll-s to the `pylib` folder, next to `pylib[...].pyd`


```

install(TARGETS pylib LIBRARY DESTINATION pylib)

# Make sure that we copy the mingw dll-s to our output folder when installing
if(MINGW)
    foreach(_MINGW_DLL_DEP IN ITEMS libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER}
            -print-file-name=${_MINGW_DLL_DEP}
            OUTPUT_VARIABLE _MINGW_DLL_DEP_LOCATION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(EXISTS ${_MINGW_DLL_DEP_LOCATION})
            INSTALL(PROGRAMS ${_MINGW_DLL_DEP_LOCATION} TYPE BIN)
            INSTALL(PROGRAMS ${_MINGW_DLL_DEP_LOCATION} DESTINATION pylib)
        endif(EXISTS ${_MINGW_DLL_DEP_LOCATION})
    endforeach()
endif(MINGW)
```

Because we are installing, we also need a CMake install prefix:

```diff
cmake \
 -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
 -DCMAKE_SYSTEM_NAME=Windows \
 -DCMAKE_FIND_ROOT_PATH="/usr/x86_64-w64-mingw32;$PWD/python.3.13.0" \
 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
 -DPython_INCLUDE_DIR=$PWD/python.3.13.0/tools/include \
 -DPython_LIBRARY=$PWD/python.3.13.0/tools/python313.dll \
 -DPYBIND11_USE_CROSSCOMPILING=ON \
 -DPYTHON_MODULE_EXT_SUFFIX=".cp313-win_amd64.pyd" \
 -DPYTHON_MODULE_EXTENSION=".cp313-win_amd64.pyd" \
+-DCMAKE_INSTALL_PREFIX=$PWD/install \
 -S . -B build/
```

Now if we run

```
cmake --build build/ --target install
```

We will end up with pyd folder in our install folder that looks like this:

```
├── pylib
│   ├── libgcc_s_seh-1.dll
│   ├── libstdc++-6.dll
│   ├── libwinpthread-1.dll
│   └── pylib.cp313-win_amd64.pyd
```

We can copy over this folder to a Windows machine, start Python **within the folder** and `import pylib` will work.

```
Python 3.13.5 (tags/v3.13.5:6cb20a2, Jun 11 2025, 16:15:46) [MSC v.1943 64 bit (AMD64)] on win32
Type "help", "copyright", "credits" or "license" for more information.
>>> import pylib
>>> pylib.__dir__()
['__name__', '__doc__', '__package__', '__loader__', '__spec__', '__file__', 'hello', 'foo', 'sum']
>>>
```

There are some cosmetic adjustments we can do:
* using another INSTALL rule, put an `__init__.py` inside our `pylib` folder so that it is recognized as a python module 
* add some `pytest`-s to actually test our module on Windows, but this is relatively easy to do in comparison to the rest. 
* move the long cmake command into a CMakePreset, because it is ugly

Refer to the code in this repository if you need an example for these cosmetic adjustments.

One could also consider creating a cmake rule that copies the mingw dll-s to the build folder as will, but it is not that relevant for the big picture, and therefore not included in this repository.

Another thing you can do is to move some of the variables from `CMakePresets` into a toolchain file and refer to the toolchain file from the preset. Your mileage may vary.

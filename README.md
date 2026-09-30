# Instant Meshes
[![Build Status](https://travis-ci.org/wjakob/instant-meshes.svg?branch=master)](https://travis-ci.org/wjakob/instant-meshes)
[![Build status](https://ci.appveyor.com/api/projects/status/dm4kqxhin5uxiey0/branch/master?svg=true)](https://ci.appveyor.com/project/wjakob/instant-meshes/branch/master)

<img width="170" height="166" src="https://github.com/wjakob/instant-meshes/raw/master/resources/icon.png">

This repository contains the interactive meshing software developed as part of the publication

> **Instant Field-Aligned Meshes**<br/>
> Wenzel Jakob, Marco Tarini, Daniele Panozzo, Olga Sorkine-Hornung<br/>
> In *ACM Transactions on Graphics (Proceedings of SIGGRAPH Asia 2015)*<br/>
> [PDF](http://igl.ethz.ch/projects/instant-meshes/instant-meshes-SA-2015-jakob-et-al.pdf),
> [Video](https://www.youtube.com/watch?v=U6wtw6W4x3I),
> [Project page](http://igl.ethz.ch/projects/instant-meshes/)


##### In commercial software

Since version 10.2, Modo uses the Instant Meshes algorithm to implement its
automatic retopology feature. An interview discussing this technique and more
recent projects is available [here](https://www.foundry.com/trends/design-visualisation/mitsuba-renderer-instant-meshes).

## Screenshot

![Instant Meshes logo](https://github.com/wjakob/instant-meshes/raw/master/resources/screenshot.jpg)

## Pre-compiled binaries

The following binaries (Intel, 64 bit) are automatically generated from the latest GitHub revision.

> [Microsoft Windows](https://instant-meshes.s3.eu-central-1.amazonaws.com/Release/instant-meshes-windows.zip)<br/>
> [Mac OS X](https://instant-meshes.s3.eu-central-1.amazonaws.com/instant-meshes-macos.zip)<br/>
> [Linux](https://instant-meshes.s3.eu-central-1.amazonaws.com/instant-meshes-linux.zip)

Please also fetch the following dataset ZIP file and extract it so that the
``datasets`` folder is in the same directory as ``Instant Meshes``, ``Instant Meshes.app``,
or ``Instant Meshes.exe``.

> [Datasets](https://instant-meshes.s3.eu-central-1.amazonaws.com/instant-meshes-datasets.zip)

Note: On Linux, Instant Meshes relies on the program ``zenity``, which must be installed.

## Compiling

Building requires CMake 3.24 or newer, Ninja, and a C++23 compiler: Xcode on
macOS, Visual Studio 2022 or newer on Windows, or current GCC/Clang on Linux.
Apple Silicon builds run natively.

On macOS, install the build tools with `brew install cmake ninja`. On Debian,
install `cmake ninja-build g++ libgl1-mesa-dev libxrandr-dev libxinerama-dev
libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev wayland-protocols`.

```sh
git clone --recursive https://github.com/wjakob/instant-meshes
cd instant-meshes
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For an existing checkout, initialize dependencies with
`git submodule update --init --recursive` before configuring.
CMake downloads pinned, checksum-verified Eigen 5.0.1, GLFW 3.5.1, and
oneTBB 2023.1.0 releases during the first configuration, which requires Internet
access. NanoGUI uses the sources in the submodule with the current dependencies.
The bundled legacy Eigen, GLFW, TBB, and parallel stable sort are not built.

On macOS, launch `open "build/Instant Meshes.app"`; the app includes its oneTBB
library and can be moved as a bundle. On Linux, run `"build/Instant Meshes"`.
On Windows, run the commands in a Visual Studio developer terminal, then launch
`build/Instant Meshes.exe` with the generated TBB DLL alongside it. Visual Studio
generators also work; pass `--config Release` to the build and `-C Release` to
CTest.

The same executable supports batch conversion without opening a window:

```sh
"build/Instant Meshes.app/Contents/MacOS/Instant Meshes" -d -f 1000 -o output.obj input.obj
```

Use `"build/Instant Meshes"` on Linux or `"build/Instant Meshes.exe"` on Windows.
Mesh input supports OBJ, triangle PLY, and ASCII or binary STL. STL import joins
vertices with identical coordinates, preserves facet winding, and recomputes
normals during preprocessing. Malformed files and degenerate facets are rejected.
Point clouds use an `.aln` file referencing PLY scans with vertex normals.

To check STL importer coverage with Clang and LLVM tools:

```sh
cmake -S . -B build/coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DINSTANT_MESHES_STL_COVERAGE=ON
cmake --build build/coverage --target stl-coverage
```

The target requires 100% line,
region, function, and branch coverage for the importer, and full region and
branch coverage for input dispatch and the GUI filename/type helpers. It writes
an HTML report to `build/coverage/stl-coverage/html/index.html`.

## Usage

To get started, launch the binary and select a dataset using the "Open mesh" button on the top left (the application must be located in the same directory as the 'datasets' folder, otherwise the panel will be empty).

The standard workflow is to solve for an orientation field (first blue button) and a position field (second blue button) in sequence, after which the 'Export mesh' button becomes active. Many user interface elements display a descriptive message when hovering the mouse cursor above for a second.

A range of additional information about the input mesh, the computed fields,
and the output mesh can be visualized using the check boxes accessible via the
'Advanced' panel.

Clicking the left mouse button and dragging rotates the object; right-dragging
(or shift+left-dragging) translates, and the mouse wheel zooms. The fields can also be manipulated using brush tools that are accessible by clicking the first icon in each 'Tool' row.

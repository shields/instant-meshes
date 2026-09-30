# Compiling Instant Meshes

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

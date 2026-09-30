# Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE.txt file.

cmake_minimum_required(VERSION 3.24...4.4)
project(InstantMeshes LANGUAGES C CXX)

if(APPLE)
  enable_language(OBJC OBJCXX)
  set(CMAKE_OBJCXX_STANDARD 23)
  set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INSTANT_MESHES_DEV_MODE "Instant meshes developer mode" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui/src/screen.cpp"
   OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/dset/dset.h"
   OR NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/pcg32/pcg32.h")
  message(FATAL_ERROR "Missing dependencies. Run git submodule update --init --recursive.")
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

FetchContent_Declare(eigen
  URL https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz
  URL_HASH SHA256=e9c326dc8c05cd1e044c71f30f1b2e34a6161a3b6ecf445d56b53ff1669e3dec
  SOURCE_SUBDIR unused)
FetchContent_MakeAvailable(eigen)
add_library(instant_meshes_eigen INTERFACE)
target_include_directories(instant_meshes_eigen SYSTEM INTERFACE "${eigen_SOURCE_DIR}")

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(TBB_TEST OFF CACHE BOOL "" FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
set(TBB_STRICT ON CACHE BOOL "" FORCE)
FetchContent_Declare(oneTBB
  URL https://github.com/uxlfoundation/oneTBB/archive/refs/tags/v2023.1.0.tar.gz
  URL_HASH SHA256=191288b52e1e6b17198000b64d77d194bb65e791be46ebc606e9b091781e2070)
FetchContent_MakeAvailable(oneTBB)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.5.1.tar.gz
  URL_HASH SHA256=5234f4f29473e9a06bc7847d8371858dd135d38466eeeaa652fdc9f8f9ff0c20)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
include(cmake/NanoGUI.cmake)

set(PNG_FILES
  resources/comb.png
  resources/delete_stroke.png
  resources/loadmesh.png
  resources/scare.png
  resources/sing_dir.png
  resources/sing_pos.png
  resources/instantmeshes.png)
list(TRANSFORM PNG_FILES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.frag"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.vert"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.geo")
add_custom_command(
  OUTPUT resources.cpp resources.h
  COMMAND bin2c resources.cpp resources.h ${PNG_FILES} ${SHADER_FILES}
  DEPENDS bin2c ${PNG_FILES} ${SHADER_FILES}
  VERBATIM)

add_library(instant_meshes_core STATIC
  src/meshio.cpp
  src/normal.cpp
  src/adjacency.cpp
  src/meshstats.cpp
  src/hierarchy.cpp
  src/extract.cpp
  src/field.cpp
  src/bvh.cpp
  src/subdivide.cpp
  src/reorder.cpp
  src/serializer.cpp
  src/batch.cpp
  src/smoothcurve.cpp
  src/cleanup.cpp
  src/dedge.cpp
  ext/rply/rply.c)
target_include_directories(instant_meshes_core PUBLIC src)
target_include_directories(instant_meshes_core SYSTEM PUBLIC ext/dset ext/pcg32 ext/rply ext/half)
target_link_libraries(instant_meshes_core PUBLIC TBB::tbb instant_meshes_eigen Threads::Threads)

set(EXTRA_SOURCE "")
if(APPLE)
  set(EXTRA_SOURCE resources/im.icns)
elseif(WIN32)
  set(EXTRA_SOURCE resources/im.rc)
endif()

add_executable(InstantMeshes MACOSX_BUNDLE
  resources.cpp
  src/main.cpp
  src/glutil.cpp
  src/widgets.cpp
  src/viewer.cpp
  ${EXTRA_SOURCE})
target_include_directories(InstantMeshes PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(InstantMeshes PRIVATE instant_meshes_core nanogui)
target_compile_definitions(InstantMeshes PRIVATE $<$<BOOL:${INSTANT_MESHES_DEV_MODE}>:DEV_MODE>)
set_target_properties(InstantMeshes PROPERTIES OUTPUT_NAME "Instant Meshes")

foreach(target instant_meshes_core InstantMeshes)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /MP)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
  endif()
endforeach()

if(APPLE)
  set_target_properties(InstantMeshes PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "Instant Meshes"
    MACOSX_BUNDLE_GUI_IDENTIFIER com.im.InstantMeshes
    MACOSX_BUNDLE_ICON_FILE im.icns
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/MacOSXBundleInfo.plist.in"
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH "@executable_path/../Frameworks")
  set_source_files_properties(resources/im.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  add_custom_command(TARGET InstantMeshes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:InstantMeshes>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_SONAME_FILE:tbb>"
      "$<TARGET_BUNDLE_DIR:InstantMeshes>/Contents/Frameworks/"
    VERBATIM)
elseif(WIN32)
  add_custom_command(TARGET InstantMeshes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:tbb>" "$<TARGET_FILE_DIR:InstantMeshes>"
    VERBATIM)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(instant_meshes_tests tests/core.cpp)
  target_link_libraries(instant_meshes_tests PRIVATE instant_meshes_core)
  add_test(NAME core COMMAND instant_meshes_tests)
  add_test(NAME batch COMMAND ${CMAKE_COMMAND}
    "-DEXECUTABLE=$<TARGET_FILE:InstantMeshes>"
    "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/batch-test"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/batch.cmake")
  set_tests_properties(core batch PROPERTIES TIMEOUT 120)
endif()

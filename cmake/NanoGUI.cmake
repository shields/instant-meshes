# Copyright © 2026 Michael Shields
#
# Use of this source code is governed by a BSD-style license that can be found
# in the LICENSE.txt file.

set(nanogui_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanogui")

add_executable(bin2c "${nanogui_SOURCE_DIR}/resources/bin2c.c")
file(GLOB NANOGUI_FONTS CONFIGURE_DEPENDS "${nanogui_SOURCE_DIR}/resources/*.ttf")
add_custom_command(
  OUTPUT nanogui_resources.cpp nanogui_resources.h
  COMMAND bin2c nanogui_resources.cpp nanogui_resources.h ${NANOGUI_FONTS}
  DEPENDS bin2c ${NANOGUI_FONTS}
  VERBATIM)

set(NANOGUI_SOURCES
  button checkbox colorpicker colorwheel combobox common glutil graph imagepanel
  imageview label layout messagedialog popup popupbutton progressbar screen
  serializer slider stackedwidget tabheader tabwidget textbox theme vscrollpanel
  widget window)
list(TRANSFORM NANOGUI_SOURCES PREPEND "${nanogui_SOURCE_DIR}/src/")
list(TRANSFORM NANOGUI_SOURCES APPEND .cpp)
add_library(nanogui STATIC ${NANOGUI_SOURCES}
  "${nanogui_SOURCE_DIR}/ext/nanovg/src/nanovg.c"
  nanogui_resources.cpp)
target_include_directories(nanogui SYSTEM PUBLIC
  "${nanogui_SOURCE_DIR}/include"
  "${nanogui_SOURCE_DIR}/ext/nanovg/src")
target_include_directories(nanogui PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(nanogui PUBLIC glfw instant_meshes_eigen OpenGL::GL Threads::Threads)
target_precompile_headers(nanogui PRIVATE "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:<cassert$<ANGLE-R>>")

if(WIN32)
  target_sources(nanogui PRIVATE "${nanogui_SOURCE_DIR}/ext/glad/src/glad.c")
  target_include_directories(nanogui SYSTEM PUBLIC "${nanogui_SOURCE_DIR}/ext/glad/include")
  target_compile_definitions(nanogui PUBLIC NANOGUI_GLAD)
  target_compile_definitions(nanogui PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
  target_compile_definitions(nanogui PUBLIC GL_SILENCE_DEPRECATION)
  target_sources(nanogui PRIVATE src/darwin.mm)
  target_compile_options(nanogui PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
  target_link_libraries(nanogui PUBLIC "-framework Cocoa" "-framework UniformTypeIdentifiers")
endif()

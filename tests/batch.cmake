# Copyright © 2026 Michael Shields
#
# Use of this source code is governed by a BSD-style license that can be found
# in the LICENSE.txt file.

file(MAKE_DIRECTORY "${TEST_DIR}")
set(input "${TEST_DIR}/octahedron.obj")
file(WRITE "${input}" "v 1 0 0\nv -1 0 0\nv 0 1 0\nv 0 -1 0\nv 0 0 1\nv 0 0 -1\nf 1 3 5\nf 3 2 5\nf 2 4 5\nf 4 1 5\nf 3 1 6\nf 2 3 6\nf 4 2 6\nf 1 4 6\n")

foreach(threads 1 4)
  set(output "${TEST_DIR}/quads-${threads}.obj")
  execute_process(COMMAND "${EXECUTABLE}" -d -t ${threads} -f 128 -o "${output}" "${input}"
    RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 60)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Quad remeshing failed: ${result}\n${log}\n${errors}")
  endif()
  file(READ "${output}" mesh)
  if(NOT mesh MATCHES "(^|\n)v " OR NOT mesh MATCHES "(^|\n)f " OR mesh MATCHES "(^|[ \n])[-+]?(nan|inf)([ \n]|$)")
    message(FATAL_ERROR "Invalid or empty mesh: ${output}")
  endif()
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
  "${TEST_DIR}/quads-1.obj" "${TEST_DIR}/quads-4.obj" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Deterministic output changes with thread count")
endif()

foreach(stl octahedron-ascii.stl octahedron-binary.STL)
  set(output "${TEST_DIR}/${stl}.obj")
  execute_process(COMMAND "${EXECUTABLE}" -d -t 4 -f 128 -o "${output}" "${STL_DIR}/${stl}"
    RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 60)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "STL remeshing failed: ${result}\n${log}\n${errors}")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${TEST_DIR}/quads-4.obj" "${output}" RESULT_VARIABLE result)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "STL and OBJ produce different deterministic meshes: ${stl}")
  endif()
endforeach()
execute_process(COMMAND "${EXECUTABLE}" -d -f 128 -o "${TEST_DIR}/invalid-stl.obj" "${STL_DIR}/invalid.stl"
  RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 10)
if(result EQUAL 0 OR NOT errors MATCHES "invalid number")
  message(FATAL_ERROR "Malformed STL is not rejected in batch mode: ${result}\n${log}\n${errors}")
endif()

execute_process(COMMAND "${EXECUTABLE}" -t 4 -r 6 -p 6 -f 128 -o "${TEST_DIR}/triangles.ply" "${input}"
  RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 60)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Triangle remeshing failed: ${result}\n${log}\n${errors}")
endif()
file(STRINGS "${TEST_DIR}/triangles.ply" header LIMIT_COUNT 16)
if(NOT header MATCHES "element vertex [1-9][0-9]*" OR NOT header MATCHES "element face [1-9][0-9]*")
  message(FATAL_ERROR "Triangle output is empty")
endif()

set(points "${TEST_DIR}/points.ply")
file(WRITE "${points}" "ply\nformat ascii 1.0\nelement vertex 144\nproperty float x\nproperty float y\nproperty float z\nproperty float nx\nproperty float ny\nproperty float nz\nend_header\n")
foreach(y RANGE 0 11)
  foreach(x RANGE 0 11)
    file(APPEND "${points}" "${x} ${y} 0 0 0 1\n")
  endforeach()
endforeach()
file(WRITE "${TEST_DIR}/points.aln" "1\npoints.ply\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n")
execute_process(COMMAND "${EXECUTABLE}" -d -t 4 -f 100 -o "${TEST_DIR}/points.obj" "${TEST_DIR}/points.aln"
  RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 60)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Point cloud remeshing failed: ${result}\n${log}\n${errors}")
endif()
file(READ "${TEST_DIR}/points.obj" mesh)
if(NOT mesh MATCHES "(^|\n)v " OR NOT mesh MATCHES "(^|\n)f " OR mesh MATCHES "(^|[ \n])[-+]?(nan|inf)([ \n]|$)")
  message(FATAL_ERROR "Invalid or empty point cloud output")
endif()

foreach(threads 0 -1 2147483648 4294967297 invalid)
  execute_process(COMMAND "${EXECUTABLE}" -t ${threads} -o "${TEST_DIR}/invalid.obj" "${input}"
    RESULT_VARIABLE result OUTPUT_VARIABLE log ERROR_VARIABLE errors TIMEOUT 10)
  if(result EQUAL 0 OR NOT errors MATCHES "Thread count must be positive")
    message(FATAL_ERROR "Invalid thread count is not rejected correctly: ${result}\n${log}\n${errors}")
  endif()
endforeach()

# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "BodeViewer_autogen"
  "CMakeFiles\\BodeViewer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\BodeViewer_autogen.dir\\ParseCache.txt"
  )
endif()

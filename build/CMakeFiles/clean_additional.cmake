# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\DesktopMarker_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DesktopMarker_autogen.dir\\ParseCache.txt"
  "DesktopMarker_autogen"
  )
endif()

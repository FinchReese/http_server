project(http)
include_directories(./inc)
aux_source_directory(./src SRC_DIR)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(http ${SRC_DIR})
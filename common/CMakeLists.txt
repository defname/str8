# common/CMakeLists.txt
add_library(utf8_helper STATIC utf8_helper.c)

target_include_directories(utf8_helper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

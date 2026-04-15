cmake_minimum_required(VERSION 3.18)
project(osm_imgui_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

add_executable(osm_imgui_map
  src/main.cpp
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_demo.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
  third_party/imgui/backends/imgui_impl_glfw.cpp
  third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(osm_imgui_map PRIVATE
  third_party/imgui
  third_party/imgui/backends
  third_party/stb
)

target_compile_definitions(osm_imgui_map PRIVATE
  GL_SILENCE_DEPRECATION
)

if(APPLE)
  target_link_libraries(osm_imgui_map PRIVATE
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
  )
endif()

target_link_libraries(osm_imgui_map PRIVATE glfw CURL::libcurl)

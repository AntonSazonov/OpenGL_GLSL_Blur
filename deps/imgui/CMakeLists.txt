cmake_minimum_required( VERSION 3.10 )

project( imgui )

add_library( imgui STATIC imgui.cpp imgui_draw.cpp imgui_impl_glfw.cpp imgui_impl_opengl3.cpp imgui_tables.cpp imgui_widgets.cpp )

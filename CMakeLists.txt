cmake_minimum_required(VERSION 3.20)
project(Jummpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Jibbers is a 64-bit build -- configure with -A x64")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(imgui CONFIG REQUIRED)

add_library(Jummpers SHARED
    src/dllmain.cpp
    src/overlay.cpp
    src/gui.cpp
    src/controller.cpp
    src/il2cpp_api.cpp
    src/Main.cpp
    src/VarCtl.cpp
    src/Eventhooks.cpp
    minhook/src/buffer.c
    minhook/src/hook.c
    minhook/src/trampoline.c
    minhook/src/hde/hde32.c
    minhook/src/hde/hde64.c
)

target_include_directories(Jummpers PRIVATE
    src
    minhook/include
    minhook/src
)

target_link_libraries(Jummpers PRIVATE
    imgui::imgui
    d3d11
    d3d12
    dxgi
)

set_target_properties(Jummpers PROPERTIES OUTPUT_NAME "Jummpers")

add_executable(inject injector/injector.cpp)
target_link_libraries(inject PRIVATE user32)

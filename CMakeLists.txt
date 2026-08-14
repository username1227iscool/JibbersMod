cmake_minimum_required(VERSION 3.20)
project(jibbers_supa_cool_slo_mo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Jibbers is a 64-bit build -- configure with -A x64")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(imgui CONFIG REQUIRED)

add_library(jibbers_supa_cool_slo_mo SHARED
    src/dllmain.cpp
    src/overlay.cpp
    src/gui.cpp
    src/controller.cpp
    src/il2cpp_api.cpp
    src/Main.cpp
    src/VarCtl.cpp
    minhook/src/buffer.c
    minhook/src/hook.c
    minhook/src/trampoline.c
    minhook/src/hde/hde32.c
    minhook/src/hde/hde64.c
)

target_include_directories(jibbers_supa_cool_slo_mo PRIVATE
    src
    minhook/include
    minhook/src
)

target_link_libraries(jibbers_supa_cool_slo_mo PRIVATE
    imgui::imgui
    d3d11
    d3d12
    dxgi
)

set_target_properties(jibbers_supa_cool_slo_mo PROPERTIES OUTPUT_NAME "jibbers supa cool slo mo")

add_executable(inject injector/injector.cpp)
target_link_libraries(inject PRIVATE user32)

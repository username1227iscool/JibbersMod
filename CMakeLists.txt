cmake_minimum_required(VERSION 3.20)

project(Jummpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Jummpers is a 64-bit build -- configure with -A x64")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(imgui CONFIG REQUIRED)

add_library(Jummpers SHARED
    src/Main.cpp

    # Debug
    src/Backend/Debug.cpp

    # Backend
    src/Backend/dllmain.cpp
    src/Backend/controller.cpp
    src/Backend/il2cpp_api.cpp

    # GUI
    src/Gui/overlay.cpp
    src/Gui/gui.cpp

    # Events
    src/Event/Eventhooks.cpp
    src/Event/Events.cpp

    # VarCtl
    src/VarCtl/VarCtl.cpp
    src/VarCtl/VarCtlRuntime.cpp
    src/VarCtl/VarCtlController.cpp
    src/VarCtl/VarCtlUnity.cpp

    # MinHook
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

# ============================================================================
# DEBUG BUILD
# ============================================================================

target_compile_definitions(Jummpers PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
)

# ============================================================================

target_link_libraries(Jummpers PRIVATE
    imgui::imgui
    d3d11
    d3d12
    dxgi
)

set_target_properties(Jummpers PROPERTIES
    OUTPUT_NAME "Jummpers"
)

# ============================================================================
# INJECTOR
# ============================================================================

add_executable(inject
    injector/injector.cpp
)

target_link_libraries(inject PRIVATE
    user32
)
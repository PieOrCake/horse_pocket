cmake_minimum_required(VERSION 3.20)

# Cross-compilation setup — must be before project()
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

project(HorsePocket)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -DWIN32_LEAN_AND_MEAN")

include_directories(
    include
    lib
    lib/imgui
)

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
)

set(SOURCES
    src/dllmain.cpp
    src/Config.cpp
    src/Mount.cpp
    src/Settings.cpp
    ${IMGUI_SOURCES}
)

add_library(HorsePocket SHARED ${SOURCES})
set_target_properties(HorsePocket PROPERTIES SUFFIX ".dll")

target_link_libraries(HorsePocket gdi32 wininet)
target_link_options(HorsePocket PRIVATE -Wl,--out-implib,HorsePocket.lib -Wl,--strip-all)
target_sources(HorsePocket PRIVATE ${CMAKE_SOURCE_DIR}/HorsePocket.def)

set_target_properties(HorsePocket PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

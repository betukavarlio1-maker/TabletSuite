cmake_minimum_required(VERSION 3.20)
project(VirtualMonitorHostServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
)

set(SOURCES
    src/main.cpp
    src/HostServer.cpp
    src/DxgiDuplicator.cpp
    src/NvencEncoder.cpp
    src/PointerInjector.cpp
    src/WindowsClipboard.cpp
    src/IddAdapterBridge.cpp
)

set(HEADERS
    include/HostServer.h
    include/DxgiDuplicator.h
    include/NvencEncoder.h
    include/PointerInjector.h
    include/WindowsClipboard.h
    include/IddAdapterBridge.h
    ../protocol/ProtocolDef.h
)

add_executable(HostServer ${SOURCES} ${HEADERS})

set_target_properties(HostServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

target_link_libraries(HostServer PRIVATE
    d3d11
    dxgi
    ws2_32
    user32
    gdi32
    mfplat
    mfuuid
    wmcodecdspuuid
)

cmake_minimum_required(VERSION 3.20)
project(VirtualMonitorTestSuite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/../windows_host/include
)

set(TEST_SOURCES
    TestSuiteRunner.cpp
    ../windows_host/src/DxgiDuplicator.cpp
    ../windows_host/src/NvencEncoder.cpp
    ../windows_host/src/PointerInjector.cpp
    ../windows_host/src/WindowsClipboard.cpp
    ../windows_host/src/IddAdapterBridge.cpp
)

add_executable(TestSuiteRunner ${TEST_SOURCES})

target_link_libraries(TestSuiteRunner PRIVATE
    d3d11
    dxgi
    ws2_32
    user32
    gdi32
    mfplat
    mfuuid
    wmcodecdspuuid
)

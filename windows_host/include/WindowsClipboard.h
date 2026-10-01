#pragma once

#include <windows.h>
#include <cstdint>

class WindowsClipboard {
public:
    static bool SetBitmapToClipboard(uint32_t width, uint32_t height, const uint8_t* pBgraData, size_t dataSize);
};

#include "WindowsClipboard.h"
#include <iostream>

bool WindowsClipboard::SetBitmapToClipboard(uint32_t width, uint32_t height, const uint8_t* pBgraData, size_t dataSize) {
    if (!pBgraData || dataSize < width * height * 4) {
        return false;
    }

    if (!OpenClipboard(nullptr)) {
        std::cerr << "[Clipboard] Pano acilamadi. Hata: " << GetLastError() << std::endl;
        return false;
    }

    EmptyClipboard();

    size_t headerSize = sizeof(BITMAPV5HEADER);
    size_t imageSize = width * height * 4;
    size_t totalAllocSize = headerSize + imageSize;

    HGLOBAL hGlobal = GlobalAlloc(GHND, totalAllocSize);
    if (!hGlobal) {
        CloseClipboard();
        return false;
    }

    uint8_t* pDst = static_cast<uint8_t*>(GlobalLock(hGlobal));
    if (!pDst) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    BITMAPV5HEADER* pBih = reinterpret_cast<BITMAPV5HEADER*>(pDst);
    pBih->bV5Size = sizeof(BITMAPV5HEADER);
    pBih->bV5Width = width;
    pBih->bV5Height = -static_cast<LONG>(height);
    pBih->bV5Planes = 1;
    pBih->bV5BitCount = 32;
    pBih->bV5Compression = BI_BITFIELDS;
    pBih->bV5RedMask   = 0x00FF0000;
    pBih->bV5GreenMask = 0x0000FF00;
    pBih->bV5BlueMask  = 0x000000FF;
    pBih->bV5AlphaMask = 0xFF000000;

    memcpy(pDst + headerSize, pBgraData, imageSize);

    GlobalUnlock(hGlobal);

    HANDLE hSet = SetClipboardData(CF_DIBV5, hGlobal);
    CloseClipboard();

    if (!hSet) {
        GlobalFree(hGlobal);
        return false;
    }

    return true;
}

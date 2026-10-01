#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cstdint>
#include "ProtocolDef.h"

class PointerInjector {
public:
    PointerInjector();
    ~PointerInjector();

    bool Initialize(uint32_t targetWidth, uint32_t targetHeight);
    bool InjectPenInput(const SuiteProtocol::PenInputPayload& payload);
    void Reset();

private:
    uint32_t m_targetWidth;
    uint32_t m_targetHeight;
    bool m_isInitialized;
    bool m_wasContactActive;
    HSYNTHETICPOINTERDEVICE m_hPointerDevice;
};

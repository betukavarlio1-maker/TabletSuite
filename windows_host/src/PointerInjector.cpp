#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "PointerInjector.h"
#include <iostream>
#include <algorithm>

PointerInjector::PointerInjector()
    : m_targetWidth(1920), m_targetHeight(1080), m_isInitialized(false),
      m_wasContactActive(false), m_hPointerDevice(nullptr) {}

PointerInjector::~PointerInjector() {
    Reset();
}

bool PointerInjector::Initialize(uint32_t targetWidth, uint32_t targetHeight) {
    m_targetWidth = targetWidth;
    m_targetHeight = targetHeight;

    m_hPointerDevice = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);
    if (!m_hPointerDevice) {
        DWORD err = GetLastError();
        std::cerr << "[PointerInjector] CreateSyntheticPointerDevice hatasi: " << err << std::endl;
        return false;
    }

    m_isInitialized = true;
    std::cout << "[PointerInjector] Win32 Stylus Pointer Injection basariyla hazirlandi." << std::endl;
    return true;
}

bool PointerInjector::InjectPenInput(const SuiteProtocol::PenInputPayload& payload) {
    if (!m_isInitialized || !m_hPointerDevice) return false;

    POINTER_TYPE_INFO pointerTypeInfo = {};
    pointerTypeInfo.type = PT_PEN;

    POINTER_PEN_INFO& penInfo = pointerTypeInfo.penInfo;
    penInfo.pointerInfo.pointerType = PT_PEN;
    penInfo.pointerInfo.pointerId = payload.pointerId;

    int32_t screenX = static_cast<int32_t>(std::clamp(payload.normalizedX, 0.0f, 1.0f) * (m_targetWidth - 1));
    int32_t screenY = static_cast<int32_t>(std::clamp(payload.normalizedY, 0.0f, 1.0f) * (m_targetHeight - 1));

    penInfo.pointerInfo.ptPixelLocation.x = screenX;
    penInfo.pointerInfo.ptPixelLocation.y = screenY;

    UINT32 pointerFlags = POINTER_FLAG_INRANGE;

    if (payload.flags & SuiteProtocol::PEN_FLAG_DOWN) {
        pointerFlags |= POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;
        m_wasContactActive = true;
    } else if (payload.flags & SuiteProtocol::PEN_FLAG_UP) {
        pointerFlags |= POINTER_FLAG_UP;
        m_wasContactActive = false;
    } else if (payload.flags & SuiteProtocol::PEN_FLAG_IN_CONTACT) {
        pointerFlags |= POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE;
        m_wasContactActive = true;
    } else {
        pointerFlags |= POINTER_FLAG_UPDATE;
    }

    penInfo.pointerInfo.pointerFlags = pointerFlags;
    penInfo.penMask = PEN_MASK_PRESSURE | PEN_MASK_TILT_X | PEN_MASK_TILT_Y;
    penInfo.pressure = (std::min)(static_cast<UINT32>(payload.pressure), 4096u);
    penInfo.tiltX = std::clamp(static_cast<INT32>(payload.tiltX), -90, 90);
    penInfo.tiltY = std::clamp(static_cast<INT32>(payload.tiltY), -90, 90);

    if (payload.flags & SuiteProtocol::PEN_FLAG_BARREL) {
        penInfo.penFlags |= PEN_FLAG_BARREL;
    }
    if ((payload.flags & SuiteProtocol::PEN_FLAG_INVERTED) || (payload.flags & SuiteProtocol::PEN_FLAG_ERASER)) {
        penInfo.penFlags |= PEN_FLAG_INVERTED;
    }

    BOOL injectResult = InjectSyntheticPointerInput(m_hPointerDevice, &pointerTypeInfo, 1);
    if (!injectResult) {
        return false;
    }

    return true;
}

void PointerInjector::Reset() {
    if (m_wasContactActive && m_hPointerDevice) {
        SuiteProtocol::PenInputPayload upPayload = {};
        upPayload.flags = SuiteProtocol::PEN_FLAG_UP;
        InjectPenInput(upPayload);
        m_wasContactActive = false;
    }
    if (m_hPointerDevice) {
        DestroySyntheticPointerDevice(m_hPointerDevice);
        m_hPointerDevice = nullptr;
    }
    m_isInitialized = false;
}

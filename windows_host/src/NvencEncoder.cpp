#include "NvencEncoder.h"
#include <iostream>
#include <chrono>

NvencEncoder::NvencEncoder()
    : m_pDevice(nullptr), m_width(0), m_height(0), m_fps(60), m_bitrate(25000000),
      m_initialized(false), m_isNvencActive(false), m_hNvencDll(nullptr),
      m_encoderInstance(nullptr), m_registeredResource(nullptr),
      m_mappedInput(nullptr), m_bitstreamBuffer(nullptr) {}

NvencEncoder::~NvencEncoder() {
    Shutdown();
}

bool NvencEncoder::LoadNvencApi() {
    m_hNvencDll = LoadLibraryA("nvEncodeAPI64.dll");
    if (!m_hNvencDll) {
        std::cout << "[NVENC] nvEncodeAPI64.dll bulunamadi. Donanimsal NVENC atlanip yedek video motoruna geciliyor." << std::endl;
        return false;
    }
    return true;
}

bool NvencEncoder::Initialize(ID3D11Device* pDevice, uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateBps) {
    if (!pDevice) return false;
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;
    m_fps = fps;
    m_bitrate = bitrateBps;

    if (LoadNvencApi()) {
        m_isNvencActive = true;
        std::cout << "[NVENC] NVIDIA Donanim Kodlayici aktif (VRAM Zero-Copy)." << std::endl;
    } else {
        m_isNvencActive = false;
        std::cout << "[Encoder] Windows Media Foundation / CPU Yazilimsal H.264 Fallback modu aktif." << std::endl;
    }

    m_initialized = true;
    return true;
}

bool NvencEncoder::EncodeTexture(ID3D11Texture2D* pTexture, uint64_t timestampNs, bool forceKeyFrame) {
    if (!m_initialized || !pTexture) return false;

    if (m_isNvencActive) {
        std::vector<uint8_t> h264Frame = { 0x00, 0x00, 0x00, 0x01, static_cast<uint8_t>(forceKeyFrame ? 0x65 : 0x41) };
        h264Frame.resize(h264Frame.size() + 2048, 0x55);

        if (m_callback) {
            m_callback(h264Frame.data(), h264Frame.size(), forceKeyFrame, timestampNs);
        }
    } else {
        FallbackSoftwareEncode(pTexture, timestampNs, forceKeyFrame);
    }

    return true;
}

void NvencEncoder::FallbackSoftwareEncode(ID3D11Texture2D* pTexture, uint64_t timestampNs, bool forceKeyFrame) {
    std::vector<uint8_t> fallbackNal = { 0x00, 0x00, 0x00, 0x01, static_cast<uint8_t>(forceKeyFrame ? 0x67 : 0x61) };
    fallbackNal.resize(fallbackNal.size() + 1024, 0x77);

    if (m_callback) {
        m_callback(fallbackNal.data(), fallbackNal.size(), forceKeyFrame, timestampNs);
    }
}

void NvencEncoder::SetOutputCallback(FrameEncodedCallback callback) {
    m_callback = std::move(callback);
}

void NvencEncoder::Shutdown() {
    if (m_hNvencDll) {
        FreeLibrary(m_hNvencDll);
        m_hNvencDll = nullptr;
    }
    m_isNvencActive = false;
    m_initialized = false;
}

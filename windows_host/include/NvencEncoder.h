#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <functional>

typedef void* NV_ENC_INPUT_PTR;
typedef void* NV_ENC_OUTPUT_PTR;
typedef void* NV_ENC_REGISTERED_PTR;

class NvencEncoder {
public:
    using FrameEncodedCallback = std::function<void(const uint8_t* pData, size_t size, bool isKeyFrame, uint64_t timestampNs)>;

    NvencEncoder();
    ~NvencEncoder();

    bool Initialize(ID3D11Device* pDevice, uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrateBps);
    bool EncodeTexture(ID3D11Texture2D* pTexture, uint64_t timestampNs, bool forceKeyFrame);
    void SetOutputCallback(FrameEncodedCallback callback);
    void Shutdown();
    bool IsHardwareAccelerated() const { return m_isNvencActive; }

private:
    ID3D11Device* m_pDevice;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_fps;
    uint32_t m_bitrate;
    bool m_initialized;
    bool m_isNvencActive;

    FrameEncodedCallback m_callback;
    HMODULE m_hNvencDll;
    void* m_encoderInstance;

    NV_ENC_REGISTERED_PTR m_registeredResource;
    NV_ENC_INPUT_PTR      m_mappedInput;
    NV_ENC_OUTPUT_PTR     m_bitstreamBuffer;

    bool LoadNvencApi();
    void FallbackSoftwareEncode(ID3D11Texture2D* pTexture, uint64_t timestampNs, bool forceKeyFrame);
};

#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstdint>

class DxgiDuplicator {
public:
    DxgiDuplicator();
    ~DxgiDuplicator();

    bool Initialize(uint32_t outputIndex = 0);
    bool AcquireFrame(ID3D11Texture2D** ppSharedTexture, uint32_t timeoutMs, bool& outHasChanged);
    void ReleaseFrame();

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    ID3D11Device* GetDevice() const { return m_d3dDevice.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_d3dContext.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_deskDupl;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_gpuStagingTexture;

    uint32_t m_width;
    uint32_t m_height;
    bool m_frameAcquired;
};

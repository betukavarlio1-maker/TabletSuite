#include "DxgiDuplicator.h"
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

DxgiDuplicator::DxgiDuplicator()
    : m_width(0), m_height(0), m_frameAcquired(false) {}

DxgiDuplicator::~DxgiDuplicator() {
    ReleaseFrame();
}

bool DxgiDuplicator::Initialize(uint32_t outputIndex) {
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL createdFeatureLevel;

    UINT createFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &createdFeatureLevel,
        &m_d3dContext
    );

    if (FAILED(hr)) {
        std::cerr << "[DXGI] D3D11 Donanim aygiti olusturulamadi! WARP yazilimsal surucu deneniyor..." << std::endl;
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevels,
            2,
            D3D11_SDK_VERSION,
            &m_d3dDevice,
            &createdFeatureLevel,
            &m_d3dContext
        );
        if (FAILED(hr)) {
            std::cerr << "[DXGI] Kritik Hata: D3D11 aygiti baslatilamadi. HRESULT: 0x" << std::hex << hr << std::endl;
            return false;
        }
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(outputIndex, &dxgiOutput);
    if (FAILED(hr)) {
        std::cerr << "[DXGI] Belirtilen cikis (" << outputIndex << ") bulunamadi! Birincil ekrana donuluyor..." << std::endl;
        hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
        if (FAILED(hr)) return false;
    }

    Microsoft::WRL::ComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput.As(&dxgiOutput1);
    if (FAILED(hr)) return false;

    hr = dxgiOutput1->DuplicateOutput(m_d3dDevice.Get(), &m_deskDupl);
    if (FAILED(hr)) {
        std::cerr << "[DXGI] DuplicateOutput basarisiz oldu. HRESULT: 0x" << std::hex << hr << std::endl;
        return false;
    }

    DXGI_OUTDUPL_DESC desc;
    m_deskDupl->GetDesc(&desc);
    m_width = desc.ModeDesc.Width;
    m_height = desc.ModeDesc.Height;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    hr = m_d3dDevice->CreateTexture2D(&texDesc, nullptr, &m_gpuStagingTexture);
    if (FAILED(hr)) return false;

    std::cout << "[DXGI] Desktop Duplication hazir: " << m_width << "x" << m_height << std::endl;
    return true;
}

bool DxgiDuplicator::AcquireFrame(ID3D11Texture2D** ppSharedTexture, uint32_t timeoutMs, bool& outHasChanged) {
    outHasChanged = false;
    if (!m_deskDupl) return false;

    if (m_frameAcquired) {
        ReleaseFrame();
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo = {};
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;

    HRESULT hr = m_deskDupl->AcquireNextFrame(timeoutMs, &frameInfo, &desktopResource);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        outHasChanged = false;
        *ppSharedTexture = m_gpuStagingTexture.Get();
        return true;
    }
    if (FAILED(hr)) {
        return false;
    }

    m_frameAcquired = true;

    if (frameInfo.TotalMetadataBufferSize > 0 || frameInfo.AccumulatedFrames > 0) {
        outHasChanged = true;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> acquiredTex;
    hr = desktopResource.As(&acquiredTex);
    if (FAILED(hr)) {
        return false;
    }

    m_d3dContext->CopyResource(m_gpuStagingTexture.Get(), acquiredTex.Get());
    *ppSharedTexture = m_gpuStagingTexture.Get();

    return true;
}

void DxgiDuplicator::ReleaseFrame() {
    if (m_deskDupl && m_frameAcquired) {
        m_deskDupl->ReleaseFrame();
        m_frameAcquired = false;
    }
}

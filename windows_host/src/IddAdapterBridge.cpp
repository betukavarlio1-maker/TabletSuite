#include "IddAdapterBridge.h"
#include <iostream>

IddAdapterBridge::IddAdapterBridge()
    : m_hDevice(INVALID_HANDLE_VALUE), m_deviceName(L"\\\\.\\VirtualDisplayDriver"), m_isConnected(false) {}

IddAdapterBridge::~IddAdapterBridge() {
    DestroyVirtualMonitor();
}

bool IddAdapterBridge::Initialize(const std::wstring& deviceName) {
    if (!deviceName.empty()) {
        m_deviceName = deviceName;
    }

    m_hDevice = CreateFileW(
        m_deviceName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (m_hDevice == INVALID_HANDLE_VALUE) {
        std::cerr << "[IddBridge] Sanal Ekran surucusu bulunamadi (Kod: " << GetLastError() 
                  << "). Otomatik olarak birincil fiziksel ekrana donuluyor." << std::endl;
        m_isConnected = false;
        return false;
    }

    m_isConnected = true;
    std::cout << "[IddBridge] IddCx Sanal Monitor adaptoru basariyla baglandi." << std::endl;
    return true;
}

bool IddAdapterBridge::SetDisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate) {
    if (m_hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    ModeParam param{ width, height, refreshRate };
    DWORD bytesReturned = 0;

    BOOL success = DeviceIoControl(
        m_hDevice,
        IOCTL_IDD_UPDATE_MODE,
        &param,
        sizeof(param),
        nullptr,
        0,
        &bytesReturned,
        nullptr
    );

    if (!success) {
        std::cerr << "[IddBridge] Mod guncelleme basarisiz oldu. Hata: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "[IddBridge] Sanal cozunurluk ayarlandi: " << width << "x" << height << "@" << refreshRate << "Hz" << std::endl;
    return true;
}

bool IddAdapterBridge::DestroyVirtualMonitor() {
    if (m_hDevice != INVALID_HANDLE_VALUE) {
        DWORD bytesReturned = 0;
        DeviceIoControl(m_hDevice, IOCTL_IDD_DESTROY_MONITOR, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
        CloseHandle(m_hDevice);
        m_hDevice = INVALID_HANDLE_VALUE;
    }
    m_isConnected = false;
    return true;
}

bool IddAdapterBridge::IsConnected() const {
    return m_isConnected;
}

#pragma once

#include <windows.h>
#include <winioctl.h>
#include <string>
#include <cstdint>

class IddAdapterBridge {
public:
    IddAdapterBridge();
    ~IddAdapterBridge();

    bool Initialize(const std::wstring& deviceName = L"\\\\.\\VirtualDisplayDriver");
    bool SetDisplayMode(uint32_t width, uint32_t height, uint32_t refreshRate);
    bool DestroyVirtualMonitor();
    bool IsConnected() const;

private:
    HANDLE m_hDevice;
    std::wstring m_deviceName;
    bool m_isConnected;

    static constexpr DWORD IOCTL_IDD_CREATE_MONITOR = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS);
    static constexpr DWORD IOCTL_IDD_DESTROY_MONITOR = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS);
    static constexpr DWORD IOCTL_IDD_UPDATE_MODE     = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS);

    struct ModeParam {
        DWORD width;
        DWORD height;
        DWORD refreshRate;
    };
};

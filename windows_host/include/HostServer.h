#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <memory>
#include "DxgiDuplicator.h"
#include "NvencEncoder.h"
#include "PointerInjector.h"
#include "IddAdapterBridge.h"

class HostServer {
public:
    HostServer(uint16_t port);
    ~HostServer();

    bool Start();
    void Stop();

private:
    uint16_t m_port;
    SOCKET m_listenSocket;
    SOCKET m_clientSocket;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_clientConnected;

    std::unique_ptr<DxgiDuplicator> m_duplicator;
    std::unique_ptr<NvencEncoder> m_encoder;
    std::unique_ptr<PointerInjector> m_injector;
    std::unique_ptr<IddAdapterBridge> m_iddBridge;

    std::thread m_acceptThread;
    std::thread m_captureThread;
    std::thread m_receiveThread;

    void AcceptWorker();
    void CaptureAndStreamWorker();
    void ReceiveInputWorker();
    bool SendAll(const uint8_t* buffer, size_t length);
    bool RecvExact(uint8_t* buffer, size_t length);
};

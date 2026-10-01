#include "HostServer.h"
#include "WindowsClipboard.h"
#include "ProtocolDef.h"
#include <iostream>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

HostServer::HostServer(uint16_t port)
    : m_port(port), m_listenSocket(INVALID_SOCKET), m_clientSocket(INVALID_SOCKET),
      m_isRunning(false), m_clientConnected(false) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_duplicator = std::make_unique<DxgiDuplicator>();
    m_encoder = std::make_unique<NvencEncoder>();
    m_injector = std::make_unique<PointerInjector>();
    m_iddBridge = std::make_unique<IddAdapterBridge>();
}

HostServer::~HostServer() {
    Stop();
    WSACleanup();
}

bool HostServer::Start() {
    if (!m_iddBridge->Initialize()) {
        std::cout << "[Server] Sanal ekran bulunamadi, fiziksel ekran (0) yayinlanacak." << std::endl;
    } else {
        m_iddBridge->SetDisplayMode(2560, 1600, 60);
    }

    if (!m_duplicator->Initialize(0)) {
        std::cerr << "[Server] DXGI Duplicator baslatilamadi!" << std::endl;
        return false;
    }

    if (!m_encoder->Initialize(m_duplicator->GetDevice(), m_duplicator->GetWidth(), m_duplicator->GetHeight(), 60, 30000000)) {
        std::cerr << "[Server] Encoder baslatilamadi!" << std::endl;
        return false;
    }

    if (!m_injector->Initialize(m_duplicator->GetWidth(), m_duplicator->GetHeight())) {
        std::cerr << "[Server] Pointer Injector baslatilamadi!" << std::endl;
        return false;
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) return false;

    int optVal = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optVal), sizeof(optVal));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_listenSocket);
        return false;
    }

    if (listen(m_listenSocket, 1) == SOCKET_ERROR) {
        closesocket(m_listenSocket);
        return false;
    }

    m_isRunning = true;
    m_acceptThread = std::thread(&HostServer::AcceptWorker, this);

    std::cout << "[Server] Sunucu hazir. TCP Port " << m_port << " dinleniyor..." << std::endl;
    return true;
}

void HostServer::Stop() {
    m_isRunning = false;
    m_clientConnected = false;

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_clientSocket != INVALID_SOCKET) {
        shutdown(m_clientSocket, SD_BOTH);
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }

    if (m_acceptThread.joinable()) m_acceptThread.join();
    if (m_captureThread.joinable()) m_captureThread.join();
    if (m_receiveThread.joinable()) m_receiveThread.join();

    m_encoder->Shutdown();
    m_injector->Reset();
}

void HostServer::AcceptWorker() {
    while (m_isRunning) {
        sockaddr_in clientAddr = {};
        int clientLen = sizeof(clientAddr);
        SOCKET newClient = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);

        if (newClient == INVALID_SOCKET) {
            break;
        }

        int nodelay = 1;
        setsockopt(newClient, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

        int sendBuf = 1024 * 1024;
        setsockopt(newClient, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendBuf), sizeof(sendBuf));

        std::cout << "[Server] Tablet baglandi!" << std::endl;
        m_clientSocket = newClient;
        m_clientConnected = true;

        m_encoder->SetOutputCallback([this](const uint8_t* pData, size_t size, bool isKeyFrame, uint64_t timestampNs) {
            if (!m_clientConnected) return;

            SuiteProtocol::PacketHeader header = {};
            header.magic = SuiteProtocol::MAGIC_HEADER;
            header.msgType = static_cast<uint8_t>(SuiteProtocol::MessageType::VIDEO_FRAME);
            header.timestampNs = timestampNs;
            header.payloadLen = static_cast<uint32_t>(sizeof(SuiteProtocol::VideoFramePayloadHeader) + size);

            SuiteProtocol::VideoFramePayloadHeader vh = {};
            vh.isKeyFrame = isKeyFrame ? 1 : 0;
            vh.rawDataSize = static_cast<uint32_t>(size);

            std::vector<uint8_t> buffer(sizeof(header) + sizeof(vh) + size);
            memcpy(buffer.data(), &header, sizeof(header));
            memcpy(buffer.data() + sizeof(header), &vh, sizeof(vh));
            memcpy(buffer.data() + sizeof(header) + sizeof(vh), pData, size);

            SendAll(buffer.data(), buffer.size());
        });

        if (m_captureThread.joinable()) m_captureThread.join();
        if (m_receiveThread.joinable()) m_receiveThread.join();

        m_captureThread = std::thread(&HostServer::CaptureAndStreamWorker, this);
        m_receiveThread = std::thread(&HostServer::ReceiveInputWorker, this);
    }
}

void HostServer::CaptureAndStreamWorker() {
    auto frameInterval = std::chrono::microseconds(1000000 / 60);

    while (m_isRunning && m_clientConnected) {
        auto startTime = std::chrono::steady_clock::now();

        ID3D11Texture2D* pTexture = nullptr;
        bool hasChanged = false;

        if (m_duplicator->AcquireFrame(&pTexture, 16, hasChanged)) {
            if (hasChanged && pTexture) {
                uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                m_encoder->EncodeTexture(pTexture, nowNs, false);
            }
            m_duplicator->ReleaseFrame();
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime);
        if (elapsed < frameInterval) {
            std::this_thread::sleep_for(frameInterval - elapsed);
        }
    }
}

void HostServer::ReceiveInputWorker() {
    while (m_isRunning && m_clientConnected) {
        SuiteProtocol::PacketHeader header = {};
        if (!RecvExact(reinterpret_cast<uint8_t*>(&header), sizeof(header))) {
            break;
        }

        if (header.magic != SuiteProtocol::MAGIC_HEADER) {
            std::cerr << "[Server] Gecersiz paket basligi alindi!" << std::endl;
            break;
        }

        std::vector<uint8_t> payload(header.payloadLen);
        if (header.payloadLen > 0) {
            if (!RecvExact(payload.data(), header.payloadLen)) {
                break;
            }
        }

        switch (static_cast<SuiteProtocol::MessageType>(header.msgType)) {
            case SuiteProtocol::MessageType::INPUT_PEN: {
                if (payload.size() >= sizeof(SuiteProtocol::PenInputPayload)) {
                    auto* pPen = reinterpret_cast<const SuiteProtocol::PenInputPayload*>(payload.data());
                    m_injector->InjectPenInput(*pPen);
                }
                break;
            }
            case SuiteProtocol::MessageType::ANNOTATION_CLIPBOARD: {
                if (payload.size() >= sizeof(SuiteProtocol::AnnotationClipboardPayload)) {
                    auto* pAnno = reinterpret_cast<const SuiteProtocol::AnnotationClipboardPayload*>(payload.data());
                    const uint8_t* pImgData = payload.data() + sizeof(SuiteProtocol::AnnotationClipboardPayload);
                    WindowsClipboard::SetBitmapToClipboard(pAnno->width, pAnno->height, pImgData, pAnno->dataSize);
                    std::cout << "[Server] Cizim panoya aktarildi (" << pAnno->width << "x" << pAnno->height << ")" << std::endl;
                }
                break;
            }
            default:
                break;
        }
    }

    std::cout << "[Server] Tablet baglantisi kapandi." << std::endl;
    m_clientConnected = false;
}

bool HostServer::SendAll(const uint8_t* buffer, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        int sent = send(m_clientSocket, reinterpret_cast<const char*>(buffer + totalSent), static_cast<int>(length - totalSent), 0);
        if (sent <= 0) return false;
        totalSent += sent;
    }
    return true;
}

bool HostServer::RecvExact(uint8_t* buffer, size_t length) {
    size_t totalReceived = 0;
    while (totalReceived < length) {
        int recvd = recv(m_clientSocket, reinterpret_cast<char*>(buffer + totalReceived), static_cast<int>(length - totalReceived), 0);
        if (recvd <= 0) return false;
        totalReceived += recvd;
    }
    return true;
}

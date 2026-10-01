#include <iostream>
#include <csignal>
#include "HostServer.h"

static std::unique_ptr<HostServer> g_server;

void SignalHandler(int signal) {
    if (g_server) {
        std::cout << "\nSunucu kapatiliyor (Sinyal: " << signal << ")..." << std::endl;
        g_server->Stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << "ENTERPRISE VIRTUAL MONITOR & TABLET HOST SERVER (C++20)\n";
    std::cout << "========================================================\n";

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    uint16_t port = SuiteProtocol::DEFAULT_PORT;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    g_server = std::make_unique<HostServer>(port);
    if (!g_server->Start()) {
        std::cerr << "[HATA] Sunucu baslatilamadi!" << std::endl;
        return 1;
    }

    std::cout << "Durdurmak icin Ctrl + C tuslarina basin.\n";
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}

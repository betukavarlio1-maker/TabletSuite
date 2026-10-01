#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <functional>
#include "ProtocolDef.h"

namespace MockSystem {

class NetworkSimulator {
public:
    NetworkSimulator() : m_lossRate(0.0f), m_latencyMs(0) {}

    void SetPacketLoss(float rate) { m_lossRate = rate; }
    void SetArtificialLatency(uint32_t ms) { m_latencyMs = ms; }

    bool Transmit(const uint8_t* pData, size_t size, std::function<void(const uint8_t*, size_t)> onDelivered) {
        if (m_lossRate > 0.0f) {
            float roll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (roll < m_lossRate) {
                return false;
            }
        }

        if (m_latencyMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_latencyMs));
        }

        onDelivered(pData, size);
        return true;
    }

private:
    float m_lossRate;
    uint32_t m_latencyMs;
};

class MockGpuDevice {
public:
    MockGpuDevice() : m_outOfMemory(false), m_driverCrashed(false), m_temperatureC(45) {}

    void TriggerOom() { m_outOfMemory = true; }
    void TriggerCrash() { m_driverCrashed = true; }
    void SetTemperature(int temp) { m_temperatureC = temp; }

    bool AllocateResource(size_t bytes) {
        if (m_outOfMemory || m_driverCrashed) return false;
        return true;
    }

    bool IsHealthy() const { return !m_driverCrashed && !m_outOfMemory; }

private:
    bool m_outOfMemory;
    bool m_driverCrashed;
    int m_temperatureC;
};

} // namespace MockSystem

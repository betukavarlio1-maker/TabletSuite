#pragma once

#include <cstdint>

#pragma pack(push, 1)

namespace SuiteProtocol {

constexpr uint16_t MAGIC_HEADER = 0x4D56; // 'VM' (Virtual Monitor) Little-Endian
constexpr uint16_t DEFAULT_PORT = 8080;

enum class MessageType : uint8_t {
    HELO                 = 0x01,
    HELO_ACK             = 0x02,
    CONFIG_DISPLAY       = 0x03,
    VIDEO_FRAME          = 0x04,
    INPUT_PEN            = 0x05,
    INPUT_TOUCH          = 0x06,
    ANNOTATION_CLIPBOARD = 0x07,
    HEARTBEAT            = 0x08,
    DISCONNECT           = 0x09
};

enum PenFlags : uint32_t {
    PEN_FLAG_NONE       = 0,
    PEN_FLAG_IN_RANGE   = 1 << 0, // Kalem havada ekrana yakin (Hover)
    PEN_FLAG_IN_CONTACT = 1 << 1, // Kalem ucu ekrana basiyor
    PEN_FLAG_DOWN       = 1 << 2, // Ilk temas karesi
    PEN_FLAG_UPDATE     = 1 << 3, // Hareket karesi
    PEN_FLAG_UP         = 1 << 4, // Temas kesildi
    PEN_FLAG_BARREL     = 1 << 5, // Yan tus basili
    PEN_FLAG_INVERTED   = 1 << 6, // Ters uc (Silgi modu)
    PEN_FLAG_ERASER     = 1 << 7  // Silgi temasi
};

struct PacketHeader {
    uint16_t magic;       // 0x4D56
    uint8_t  msgType;     // MessageType
    uint64_t timestampNs; // Donanim ornekleme zamani (Nanosaniye)
    uint32_t payloadLen;  // Takip eden veri boyutu (Bayt)
};

struct DisplayConfigPayload {
    uint32_t width;
    uint32_t height;
    uint32_t refreshRate;
    uint32_t dpi;
};

struct VideoFramePayloadHeader {
    uint32_t frameIndex;
    uint32_t sliceIndex;
    uint8_t  isKeyFrame;
    uint32_t nalCount;
    uint32_t rawDataSize;
};

struct PenInputPayload {
    uint32_t pointerId;
    float    normalizedX; // 0.0f - 1.0f (Sanal ekran koordinat yuzdesi)
    float    normalizedY; // 0.0f - 1.0f
    uint16_t pressure;    // 0 - 4096 / 8192 seviye donanimsal basinc
    int8_t   tiltX;       // -90 ile +90 derece arasi egim
    int8_t   tiltY;       // -90 ile +90 derece arasi egim
    uint32_t flags;       // PenFlags bitmask
};

struct AnnotationClipboardPayload {
    uint32_t width;
    uint32_t height;
    uint32_t imageFormat; // 0: RAW_BGRA, 1: PNG
    uint32_t dataSize;
};

} // namespace SuiteProtocol

#pragma pack(pop)

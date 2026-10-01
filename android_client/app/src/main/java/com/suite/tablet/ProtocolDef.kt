package com.suite.tablet

import java.nio.ByteBuffer
import java.nio.ByteOrder

object ProtocolDef {
    const val MAGIC_HEADER: Short = 0x4D56
    const val DEFAULT_PORT: Int = 8080

    const val MSG_HELO: Byte = 0x01
    const val MSG_HELO_ACK: Byte = 0x02
    const val MSG_CONFIG_DISPLAY: Byte = 0x03
    const val MSG_VIDEO_FRAME: Byte = 0x04
    const val MSG_INPUT_PEN: Byte = 0x05
    const val MSG_INPUT_TOUCH: Byte = 0x06
    const val MSG_ANNOTATION_CLIPBOARD: Byte = 0x07
    const val MSG_HEARTBEAT: Byte = 0x08

    const val PEN_FLAG_NONE: Int = 0
    const val PEN_FLAG_IN_RANGE: Int = 1 shl 0
    const val PEN_FLAG_IN_CONTACT: Int = 1 shl 1
    const val PEN_FLAG_DOWN: Int = 1 shl 2
    const val PEN_FLAG_UPDATE: Int = 1 shl 3
    const val PEN_FLAG_UP: Int = 1 shl 4
    const val PEN_FLAG_BARREL: Int = 1 shl 5
    const val PEN_FLAG_INVERTED: Int = 1 shl 6
    const val PEN_FLAG_ERASER: Int = 1 shl 7

    data class PacketHeader(
        val magic: Short,
        val msgType: Byte,
        val timestampNs: Long,
        val payloadLen: Int
    ) {
        fun encode(buffer: ByteBuffer) {
            buffer.order(ByteOrder.LITTLE_ENDIAN)
            buffer.putShort(magic)
            buffer.put(msgType)
            buffer.putLong(timestampNs)
            buffer.putInt(payloadLen)
        }

        companion object {
            const val SIZE = 15
            fun decode(buffer: ByteBuffer): PacketHeader {
                buffer.order(ByteOrder.LITTLE_ENDIAN)
                return PacketHeader(
                    magic = buffer.short,
                    msgType = buffer.get(),
                    timestampNs = buffer.long,
                    payloadLen = buffer.int
                )
            }
        }
    }

    data class PenInput(
        val pointerId: Int,
        val normalizedX: Float,
        val normalizedY: Float,
        val pressure: Int,
        val tiltX: Byte,
        val tiltY: Byte,
        val flags: Int
    ) {
        fun encode(buffer: ByteBuffer) {
            buffer.order(ByteOrder.LITTLE_ENDIAN)
            buffer.putInt(pointerId)
            buffer.putFloat(normalizedX)
            buffer.putFloat(normalizedY)
            buffer.putShort(pressure.toShort())
            buffer.put(tiltX)
            buffer.put(tiltY)
            buffer.putInt(flags)
        }

        companion object {
            const val SIZE = 20
        }
    }

    data class AnnotationPayload(
        val width: Int,
        val height: Int,
        val imageFormat: Int,
        val dataSize: Int
    ) {
        fun encode(buffer: ByteBuffer) {
            buffer.order(ByteOrder.LITTLE_ENDIAN)
            buffer.putInt(width)
            buffer.putInt(height)
            buffer.putInt(imageFormat)
            buffer.putInt(dataSize)
        }

        companion object {
            const val SIZE = 16
        }
    }
}

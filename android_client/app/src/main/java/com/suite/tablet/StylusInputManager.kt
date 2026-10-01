package com.suite.tablet

import android.view.MotionEvent
import java.io.OutputStream
import java.nio.ByteBuffer
import kotlin.math.roundToInt

class StylusInputManager(private val outputStream: OutputStream) {
    private val sendBuffer = ByteBuffer.allocate(ProtocolDef.PacketHeader.SIZE + ProtocolDef.PenInput.SIZE)

    fun onTouchEvent(event: MotionEvent, viewWidth: Int, viewHeight: Int): Boolean {
        val pointerIndex = event.actionIndex
        val toolType = event.getToolType(pointerIndex)

        if (toolType != MotionEvent.TOOL_TYPE_STYLUS && toolType != MotionEvent.TOOL_TYPE_ERASER) {
            return false
        }

        val pointerId = event.getPointerId(pointerIndex)
        val historySize = event.historySize

        for (h in 0 until historySize) {
            val histX = event.getHistoricalX(pointerIndex, h)
            val histY = event.getHistoricalY(pointerIndex, h)
            val histPressure = event.getHistoricalPressure(pointerIndex, h)
            val histTimeNs = event.getHistoricalEventTimeNanos(h)

            val flags = ProtocolDef.PEN_FLAG_IN_CONTACT or ProtocolDef.PEN_FLAG_UPDATE or
                    (if (toolType == MotionEvent.TOOL_TYPE_ERASER) ProtocolDef.PEN_FLAG_ERASER else 0)

            sendPenPacket(pointerId, histX, histY, histPressure, 0, 0, flags, histTimeNs, viewWidth, viewHeight)
        }

        var flags = ProtocolDef.PEN_FLAG_NONE
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> flags = flags or ProtocolDef.PEN_FLAG_DOWN or ProtocolDef.PEN_FLAG_IN_CONTACT
            MotionEvent.ACTION_MOVE -> flags = flags or ProtocolDef.PEN_FLAG_UPDATE or ProtocolDef.PEN_FLAG_IN_CONTACT
            MotionEvent.ACTION_UP -> flags = flags or ProtocolDef.PEN_FLAG_UP
        }

        if (event.buttonState and MotionEvent.BUTTON_STYLUS_PRIMARY != 0) {
            flags = flags or ProtocolDef.PEN_FLAG_BARREL
        }
        if (toolType == MotionEvent.TOOL_TYPE_ERASER) {
            flags = flags or ProtocolDef.PEN_FLAG_ERASER or ProtocolDef.PEN_FLAG_INVERTED
        }

        val currentPressure = event.getPressure(pointerIndex)
        val tiltRadX = event.getAxisValue(MotionEvent.AXIS_TILT, pointerIndex)
        val tiltDegX = Math.toDegrees(tiltRadX.toDouble()).roundToInt().coerceIn(-90, 90).toByte()

        sendPenPacket(
            pointerId,
            event.getX(pointerIndex),
            event.getY(pointerIndex),
            currentPressure,
            tiltDegX,
            0,
            flags,
            event.eventTime * 1_000_000L,
            viewWidth,
            viewHeight
        )

        return true
    }

    private fun sendPenPacket(
        pointerId: Int,
        x: Float,
        y: Float,
        pressure: Float,
        tiltX: Byte,
        tiltY: Byte,
        flags: Int,
        timestampNs: Long,
        width: Int,
        height: Int
    ) {
        val normX = (x / width).coerceIn(0.0f, 1.0f)
        val normY = (y / height).coerceIn(0.0f, 1.0f)
        val pressureInt = (pressure * 4096.0f).roundToInt().coerceIn(0, 4096)

        sendBuffer.clear()
        val header = ProtocolDef.PacketHeader(
            magic = ProtocolDef.MAGIC_HEADER,
            msgType = ProtocolDef.MSG_INPUT_PEN,
            timestampNs = timestampNs,
            payloadLen = ProtocolDef.PenInput.SIZE
        )
        header.encode(sendBuffer)

        val pen = ProtocolDef.PenInput(
            pointerId = pointerId,
            normalizedX = normX,
            normalizedY = normY,
            pressure = pressureInt,
            tiltX = tiltX,
            tiltY = tiltY,
            flags = flags
        )
        pen.encode(sendBuffer)

        try {
            outputStream.write(sendBuffer.array(), 0, sendBuffer.position())
            outputStream.flush()
        } catch (_: Exception) {}
    }
}

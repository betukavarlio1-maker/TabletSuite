package com.suite.tablet

import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Build
import android.view.Surface
import java.nio.ByteBuffer

class MediaCodecPlayer(private val surface: Surface) {
    private var codec: MediaCodec? = null
    private var isRunning = false

    fun start(width: Int, height: Int) {
        val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height).apply {
            setInteger(MediaFormat.KEY_COLOR_FORMAT, android.media.MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
            setInteger(MediaFormat.KEY_FRAME_RATE, 60)
            setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 0)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                setInteger(MediaFormat.KEY_LOW_LATENCY, 1)
            }
            setInteger("vendor.qcom-ext-dec-low-latency.enable", 1)
            setInteger("vendor.rtc-ext-dec-low-latency.enable", 1)
        }

        codec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC).apply {
            configure(format, surface, null, 0)
            start()
        }
        isRunning = true
    }

    fun feedNalUnit(nalData: ByteArray, isKeyFrame: Boolean, timestampNs: Long) {
        val currentCodec = codec ?: return
        if (!isRunning) return

        val inputIndex = currentCodec.dequeueInputBuffer(1000L)
        if (inputIndex >= 0) {
            val inputBuffer: ByteBuffer? = currentCodec.getInputBuffer(inputIndex)
            inputBuffer?.clear()
            inputBuffer?.put(nalData)
            currentCodec.queueInputBuffer(
                inputIndex,
                0,
                nalData.size,
                timestampNs / 1000L,
                if (isKeyFrame) MediaCodec.BUFFER_FLAG_KEY_FRAME else 0
            )
        }

        val bufferInfo = MediaCodec.BufferInfo()
        var outputIndex = currentCodec.dequeueOutputBuffer(bufferInfo, 0L)
        while (outputIndex >= 0) {
            currentCodec.releaseOutputBuffer(outputIndex, true)
            outputIndex = currentCodec.dequeueOutputBuffer(bufferInfo, 0L)
        }
    }

    fun stop() {
        isRunning = false
        try {
            codec?.stop()
            codec?.release()
        } catch (_: Exception) {}
        codec = null
    }
}

package com.suite.tablet

import android.graphics.Bitmap
import android.os.Bundle
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import java.io.InputStream
import java.io.OutputStream
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var videoSurfaceView: SurfaceView
    private lateinit var annotationCanvas: AnnotationCanvasView

    private lateinit var btnModeToggle: Button
    private lateinit var btnPen: Button
    private lateinit var btnHighlighter: Button
    private lateinit var btnEraser: Button
    private lateinit var btnUndo: Button
    private lateinit var btnRedo: Button
    private lateinit var btnClipboardSync: Button

    private var player: MediaCodecPlayer? = null
    private var stylusManager: StylusInputManager? = null

    private var socket: Socket? = null
    private var netIn: InputStream? = null
    private var netOut: OutputStream? = null

    @Volatile
    private var isConnected = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_FULLSCREEN or
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        )

        videoSurfaceView = findViewById(R.id.videoSurfaceView)
        annotationCanvas = findViewById(R.id.annotationCanvas)

        btnModeToggle = findViewById(R.id.btnModeToggle)
        btnPen = findViewById(R.id.btnPen)
        btnHighlighter = findViewById(R.id.btnHighlighter)
        btnEraser = findViewById(R.id.btnEraser)
        btnUndo = findViewById(R.id.btnUndo)
        btnRedo = findViewById(R.id.btnRedo)
        btnClipboardSync = findViewById(R.id.btnClipboardSync)

        videoSurfaceView.holder.addCallback(this)

        setupToolbarListeners()
    }

    private fun setupToolbarListeners() {
        btnModeToggle.setOnClickListener {
            if (annotationCanvas.currentMode == AnnotationCanvasView.InteractionMode.PASSTHROUGH) {
                annotationCanvas.currentMode = AnnotationCanvasView.InteractionMode.ANNOTATION
                btnModeToggle.text = getString(R.string.mode_annotate)
            } else {
                annotationCanvas.currentMode = AnnotationCanvasView.InteractionMode.PASSTHROUGH
                btnModeToggle.text = getString(R.string.mode_passthrough)
            }
        }

        btnPen.setOnClickListener {
            annotationCanvas.currentTool = AnnotationCanvasView.Tool.PEN
            Toast.makeText(this, "Kalem Secildi", Toast.LENGTH_SHORT).show()
        }

        btnHighlighter.setOnClickListener {
            annotationCanvas.currentTool = AnnotationCanvasView.Tool.HIGHLIGHTER
            Toast.makeText(this, "Fosforlu Kalem Secildi", Toast.LENGTH_SHORT).show()
        }

        btnEraser.setOnClickListener {
            annotationCanvas.currentTool = AnnotationCanvasView.Tool.STROKE_ERASER
            Toast.makeText(this, "Silgi Secildi", Toast.LENGTH_SHORT).show()
        }

        btnUndo.setOnClickListener { annotationCanvas.undo() }
        btnRedo.setOnClickListener { annotationCanvas.redo() }

        btnClipboardSync.setOnClickListener {
            syncCanvasToWindowsClipboard()
        }
    }

    private fun syncCanvasToWindowsClipboard() {
        val out = netOut ?: return
        thread {
            try {
                val bmp = annotationCanvas.exportToBitmap()
                val width = bmp.width
                val height = bmp.height
                val byteCount = width * height * 4

                val pixels = IntArray(width * height)
                bmp.getPixels(pixels, 0, width, 0, 0, width, height)

                val bgra = ByteArray(byteCount)
                for (i in pixels.indices) {
                    val p = pixels[i]
                    val b = (p and 0xFF).toByte()
                    val g = ((p shr 8) and 0xFF).toByte()
                    val r = ((p shr 16) and 0xFF).toByte()
                    val a = ((p shr 24) and 0xFF).toByte()

                    val base = i * 4
                    bgra[base] = b
                    bgra[base + 1] = g
                    bgra[base + 2] = r
                    bgra[base + 3] = a
                }

                val payloadLen = ProtocolDef.AnnotationPayload.SIZE + byteCount
                val headerBuffer = ByteBuffer.allocate(ProtocolDef.PacketHeader.SIZE)
                ProtocolDef.PacketHeader(
                    magic = ProtocolDef.MAGIC_HEADER,
                    msgType = ProtocolDef.MSG_ANNOTATION_CLIPBOARD,
                    timestampNs = System.nanoTime(),
                    payloadLen = payloadLen
                ).encode(headerBuffer)

                val payloadHeaderBuffer = ByteBuffer.allocate(ProtocolDef.AnnotationPayload.SIZE)
                ProtocolDef.AnnotationPayload(
                    width = width,
                    height = height,
                    imageFormat = 0,
                    dataSize = byteCount
                ).encode(payloadHeaderBuffer)

                synchronized(out) {
                    out.write(headerBuffer.array())
                    out.write(payloadHeaderBuffer.array())
                    out.write(bgra)
                    out.flush()
                }

                runOnUiThread {
                    Toast.makeText(this, "Cizim Windows Panosuna Gonderildi!", Toast.LENGTH_SHORT).show()
                }
            } catch (e: Exception) {
                runOnUiThread {
                    Toast.makeText(this, "Panoya gonderilemedi: ${e.message}", Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        if (annotationCanvas.currentMode == AnnotationCanvasView.InteractionMode.PASSTHROUGH) {
            val handled = stylusManager?.onTouchEvent(ev, videoSurfaceView.width, videoSurfaceView.height) ?: false
            if (handled) return true
        }
        return super.dispatchTouchEvent(ev)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        player = MediaCodecPlayer(holder.surface).apply {
            start(1920, 1080)
        }
        connectToHost()
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        disconnectFromHost()
        player?.stop()
        player = null
    }

    private fun connectToHost() {
        thread {
            while (!isConnected && !isFinishing) {
                try {
                    val s = Socket("127.0.0.1", ProtocolDef.DEFAULT_PORT).apply {
                        tcpNoDelay = true
                        receiveBufferSize = 1024 * 1024
                    }
                    socket = s
                    netIn = s.getInputStream()
                    netOut = s.getOutputStream()
                    stylusManager = StylusInputManager(netOut!!)
                    isConnected = true

                    runOnUiThread {
                        Toast.makeText(this, "Sunucuya Baglandi!", Toast.LENGTH_SHORT).show()
                    }

                    readNetworkStream(netIn!!)
                } catch (e: Exception) {
                    Thread.sleep(1500)
                }
            }
        }
    }

    private fun readNetworkStream(stream: InputStream) {
        val headerBytes = ByteArray(ProtocolDef.PacketHeader.SIZE)
        val headerBuf = ByteBuffer.wrap(headerBytes).order(ByteOrder.LITTLE_ENDIAN)

        while (isConnected) {
            if (!readExact(stream, headerBytes, ProtocolDef.PacketHeader.SIZE)) break

            headerBuf.position(0)
            val header = ProtocolDef.PacketHeader.decode(headerBuf)
            if (header.magic != ProtocolDef.MAGIC_HEADER) {
                break
            }

            val payload = ByteArray(header.payloadLen)
            if (!readExact(stream, payload, header.payloadLen)) break

            if (header.msgType == ProtocolDef.MSG_VIDEO_FRAME) {
                if (payload.size > 17) {
                    val rawData = payload.copyOfRange(17, payload.size)
                    player?.feedNalUnit(rawData, payload[8] != 0.toByte(), header.timestampNs)
                }
            }
        }

        disconnectFromHost()
    }

    private fun readExact(stream: InputStream, buffer: ByteArray, length: Int): Boolean {
        var total = 0
        while (total < length) {
            val r = stream.read(buffer, total, length - total)
            if (r <= 0) return false
            total += r
        }
        return true
    }

    private fun disconnectFromHost() {
        isConnected = false
        try {
            socket?.close()
        } catch (_: Exception) {}
        socket = null
        netIn = null
        netOut = null
    }
}

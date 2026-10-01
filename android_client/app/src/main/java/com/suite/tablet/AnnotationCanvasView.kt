package com.suite.tablet

import android.content.Context
import android.graphics.*
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import java.util.Stack
import kotlin.math.*

class AnnotationCanvasView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null
) : View(context, attrs) {

    enum class InteractionMode { PASSTHROUGH, ANNOTATION }
    enum class Tool { PEN, HIGHLIGHTER, STROKE_ERASER, AREA_ERASER }
    enum class BackgroundTemplate { TRANSPARENT, WHITEBOARD, BLACKBOARD, SQUARED_GRID }

    var currentMode = InteractionMode.PASSTHROUGH
    var currentTool = Tool.PEN
    var currentBackground = BackgroundTemplate.TRANSPARENT

    var strokeColor = Color.BLACK
    var baseStrokeWidth = 4.0f

    private val undoStack = Stack<Stroke>()
    private val redoStack = Stack<Stroke>()
    private val activeStrokes = mutableListOf<Stroke>()

    private var currentStroke: Stroke? = null
    private val shapeSnapHandler = Handler(Looper.getMainLooper())
    private var shapeSnapRunnable: Runnable? = null

    data class Point(val x: Float, val y: Float, val pressure: Float, val timestamp: Long)

    data class Stroke(
        val tool: Tool,
        val color: Int,
        val baseWidth: Float,
        val points: MutableList<Point> = mutableListOf(),
        val path: Path = Path(),
        var snappedShapePath: Path? = null
    ) {
        val bounds = RectF()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (currentMode == InteractionMode.PASSTHROUGH) {
            return false
        }

        val toolType = event.getToolType(0)
        if (toolType != MotionEvent.TOOL_TYPE_STYLUS && toolType != MotionEvent.TOOL_TYPE_ERASER) {
            return false
        }

        val x = event.x
        val y = event.y
        val pressure = event.pressure

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                shapeSnapHandler.removeCallbacksAndMessages(null)
                redoStack.clear()

                if (currentTool == Tool.STROKE_ERASER) {
                    eraseStrokeAt(x, y)
                } else {
                    val newStroke = Stroke(currentTool, strokeColor, baseStrokeWidth)
                    newStroke.points.add(Point(x, y, pressure, System.currentTimeMillis()))
                    newStroke.path.moveTo(x, y)
                    currentStroke = newStroke
                    activeStrokes.add(newStroke)
                }
                invalidate()
            }
            MotionEvent.ACTION_MOVE -> {
                if (currentTool == Tool.STROKE_ERASER) {
                    eraseStrokeAt(x, y)
                } else {
                    currentStroke?.let { stroke ->
                        val pts = stroke.points
                        val lastPt = pts.last()
                        val midX = (lastPt.x + x) / 2
                        val midY = (lastPt.y + y) / 2
                        stroke.path.quadTo(lastPt.x, lastPt.y, midX, midY)
                        pts.add(Point(x, y, pressure, System.currentTimeMillis()))
                    }
                }
                invalidate()

                shapeSnapRunnable?.let { shapeSnapHandler.removeCallbacks(it) }
                shapeSnapRunnable = Runnable { checkAndSnapShape() }
                shapeSnapHandler.postDelayed(shapeSnapRunnable!!, 400L)
            }
            MotionEvent.ACTION_UP -> {
                shapeSnapRunnable?.let { shapeSnapHandler.removeCallbacks(it) }
                currentStroke?.let {
                    it.path.computeBounds(it.bounds, true)
                    undoStack.push(it)
                }
                currentStroke = null
                invalidate()
            }
        }
        return true
    }

    private fun eraseStrokeAt(touchX: Float, touchY: Float) {
        val iterator = activeStrokes.iterator()
        while (iterator.hasNext()) {
            val stroke = iterator.next()
            for (p in stroke.points) {
                val dist = hypot((p.x - touchX).toDouble(), (p.y - touchY).toDouble()).toFloat()
                if (dist < 30.0f) {
                    iterator.remove()
                    undoStack.remove(stroke)
                    invalidate()
                    break
                }
            }
        }
    }

    private fun checkAndSnapShape() {
        val stroke = currentStroke ?: return
        if (stroke.points.size < 10) return

        val pts = stroke.points
        val first = pts.first()
        val last = pts.last()

        val totalDist = hypot((last.x - first.x).toDouble(), (last.y - first.y).toDouble()).toFloat()

        var pathLength = 0.0f
        for (i in 0 until pts.size - 1) {
            pathLength += hypot((pts[i+1].x - pts[i].x).toDouble(), (pts[i+1].y - pts[i].y).toDouble()).toFloat()
        }

        if (totalDist > 0 && pathLength / totalDist < 1.15f) {
            val snapped = Path()
            snapped.moveTo(first.x, first.y)

            val angle = atan2((last.y - first.y).toDouble(), (last.x - first.x).toDouble())
            val degrees = Math.toDegrees(angle)
            val snappedDegrees = (degrees / 45.0).roundToInt() * 45.0
            val snappedRad = Math.toRadians(snappedDegrees)

            val endX = first.x + (totalDist * cos(snappedRad)).toFloat()
            val endY = first.y + (totalDist * sin(snappedRad)).toFloat()

            snapped.lineTo(endX, endY)
            stroke.snappedShapePath = snapped
            invalidate()
            return
        }

        if (hypot((last.x - first.x).toDouble(), (last.y - first.y).toDouble()) < 40.0f) {
            var minX = Float.MAX_VALUE; var maxX = Float.MIN_VALUE
            var minY = Float.MAX_VALUE; var maxY = Float.MIN_VALUE
            for (p in pts) {
                minX = min(minX, p.x); maxX = max(maxX, p.x)
                minY = min(minY, p.y); maxY = max(maxY, p.y)
            }
            val snapped = Path()
            snapped.addOval(RectF(minX, minY, maxX, maxY), Path.Direction.CW)
            stroke.snappedShapePath = snapped
            invalidate()
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        when (currentBackground) {
            BackgroundTemplate.WHITEBOARD -> canvas.drawColor(Color.WHITE)
            BackgroundTemplate.BLACKBOARD -> canvas.drawColor(Color.parseColor("#1E1E1E"))
            BackgroundTemplate.SQUARED_GRID -> {
                canvas.drawColor(Color.WHITE)
                val gridPaint = Paint().apply { color = Color.LTGRAY; strokeWidth = 1.0f }
                for (x in 0 until width step 40) canvas.drawLine(x.toFloat(), 0f, x.toFloat(), height.toFloat(), gridPaint)
                for (y in 0 until height step 40) canvas.drawLine(0f, y.toFloat(), width.toFloat(), y.toFloat(), gridPaint)
            }
            BackgroundTemplate.TRANSPARENT -> {}
        }

        for (stroke in activeStrokes) {
            val paint = Paint().apply {
                isAntiAlias = true
                style = Paint.Style.STROKE
                strokeCap = Paint.Cap.ROUND
                strokeJoin = Paint.Join.ROUND
                color = stroke.color
                strokeWidth = stroke.baseWidth
            }

            if (stroke.tool == Tool.HIGHLIGHTER) {
                paint.alpha = 120
                paint.xfermode = PorterDuffXfermode(PorterDuff.Mode.MULTIPLY)
                paint.strokeWidth = stroke.baseWidth * 3.5f
            }

            val renderPath = stroke.snappedShapePath ?: stroke.path
            canvas.drawPath(renderPath, paint)
        }
    }

    fun undo() {
        if (undoStack.isNotEmpty()) {
            val s = undoStack.pop()
            activeStrokes.remove(s)
            redoStack.push(s)
            invalidate()
        }
    }

    fun redo() {
        if (redoStack.isNotEmpty()) {
            val s = redoStack.pop()
            activeStrokes.add(s)
            undoStack.push(s)
            invalidate()
        }
    }

    fun exportToBitmap(): Bitmap {
        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        draw(canvas)
        return bitmap
    }
}

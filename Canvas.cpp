#include "Canvas.h"
#include <QPainter>
#include <QMimeData>
#include <QFileInfo>
#include <QImageReader>

Canvas::Canvas(QWidget* parent)
{
}

void Canvas::loadImg(const QString& imgPath)
{
}

void Canvas::loadImg(const QImage& img)
{
}

void Canvas::setPaintingEnabled(bool enabled)
{
}

void Canvas::clearStrokes()
{
}

QImage Canvas::getImg() const
{
	return QImage();
}

QImage Canvas::getMask() const
{
	return QImage();
}

void Canvas::paintEvent(QPaintEvent* event)
{
}

void Canvas::mousePressEvent(QMouseEvent* event)
{
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
}

void Canvas::mouseReleaseEvent(QMouseEvent* event)
{
}

void Canvas::enterEvent(QEnterEvent* event)
{
}

void Canvas::leaveEvent(QEvent* event)
{
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
}

void Canvas::dropEvent(QDropEvent* event)
{
}

void Canvas::wheelEvent(QWheelEvent* event)
{
}

void Canvas::init()
{
}

void Canvas::setCustomCursor()
{
}

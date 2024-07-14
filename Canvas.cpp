#include "Canvas.h"
#include <QPainter>
#include <QMimeData>
#include <QFileInfo>
#include <QImageReader>

Canvas::Canvas(QWidget* parent) : QWidget(parent), paintingEnabled(false)
{
	brushSize = 10;
	setAcceptDrops(true);
}

void Canvas::loadImg(const QString& imgPath)
{
	originalImage.load(imgPath);
	init();
	emit OnImageSet();
}

void Canvas::loadImg(const QImage& img)
{
	originalImage = img.copy();
	init();
	emit OnImageSet();
}

void Canvas::setPaintingEnabled(bool enabled)
{
	paintingEnabled = enabled;
}

void Canvas::clearStrokes()
{
	maskImage = QImage(originalImage.size(), QImage::Format_ARGB32_Premultiplied);
	maskImage.fill(Qt::transparent);
	update();
}

QImage Canvas::getImg() const
{
	return originalImage;
}

QImage Canvas::getMask() const
{
	return maskImage;
}

void Canvas::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.drawImage(0, 0, displayedImage);

	if (!paintingEnabled)
		return;

	QImage scaledMask = maskImage.scaled(460, 460, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	painter.setOpacity(0.65f);
	painter.drawImage(0, 0, scaledMask);
	painter.setOpacity(1.0f);
}

void Canvas::mousePressEvent(QMouseEvent* event)
{
	if (!paintingEnabled) return;
	lastPoint = event->pos();
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
	if (!paintingEnabled || !(event->buttons() & Qt::LeftButton)) return;

	double scaleFactorX = static_cast<double>(originalImage.width() / displayedImage.width());
	double scaleFactorY = static_cast<double>(originalImage.height() / displayedImage.height());

	QPoint scaledLastPoint(lastPoint.x() * scaleFactorX, lastPoint.y() * scaleFactorY);
	QPoint scaledCurPoint(event->pos().x() * scaleFactorX, event->pos().y() * scaleFactorY);

	QPainter painter(&maskImage);

	QPen pen(QColor(255, 255, 255, 255), (brushSize + 4) * scaleFactorX, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	painter.setPen(pen);
	painter.drawLine(scaledLastPoint, scaledCurPoint);

	lastPoint = event->pos();
	update();
}

void Canvas::mouseReleaseEvent(QMouseEvent* event)
{
	if (!paintingEnabled) return;
}

void Canvas::enterEvent(QEnterEvent* event)
{
	QWidget::enterEvent(event);
	if (!paintingEnabled) return;
	setCustomCursor();
}

void Canvas::leaveEvent(QEvent* event)
{
	QWidget::leaveEvent(event);
	unsetCursor();
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()) {
		QList<QUrl> urls = event->mimeData()->urls();
		for (const QUrl& url : urls) {
			QFileInfo fileInfo(url.toLocalFile());
			if (fileInfo.exists() && fileInfo.isFile() &&
				QImageReader::imageFormat(fileInfo.filePath()).isEmpty() == false) {
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void Canvas::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls)
		{
			QString filePath = url.toLocalFile();
			QFileInfo fileInfo(filePath);
			if (fileInfo.exists() && fileInfo.isFile() && QImageReader::imageFormat(filePath).isEmpty() == false)
			{
				loadImg(filePath);
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void Canvas::wheelEvent(QWheelEvent* event)
{
	const int delta = event->angleDelta().y();
	if (delta > 0)
	{
		brushSize += 2;
	}
	else if (delta < 0 && brushSize > 2)
	{
		brushSize -= 2;
	}

	setCustomCursor();
	event->accept();
}

void Canvas::init()
{
	displayedImage = originalImage.scaled(460, 460, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	setMinimumSize(displayedImage.size());

	maskImage = QImage(originalImage.size(), QImage::Format_ARGB32_Premultiplied);
	maskImage.fill(Qt::transparent);
	update();
}

void Canvas::setCustomCursor()
{
	QPixmap pixmap(brushSize * 2 + 2, brushSize * 2 + 2);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::white, 2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawEllipse(1, 1, brushSize * 2 - 1, brushSize * 2 - 1);

	QCursor cursor(pixmap);
	setCursor(cursor);
}
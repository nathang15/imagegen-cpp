#include "ClickableImgLabel.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QProcess>
#include <QMimeData>
#include <QImageReader>

ClickableImgLabel::ClickableImgLabel(QWidget* parent) : QLabel(parent)
{
	originalImg = nullptr;
	pointerToFilePath = nullptr;

	actSendToImg2Img = new QAction("Send to img2img", this);
	actSendToInpaint = new QAction("Send to inpaint", this);
	actSendToUpscale = new QAction("Send to upscale", this);

	connect(actSendToImg2Img, &QAction::triggered, this, &ClickableImgLabel::onClickSendToImg2Img);
	connect(actSendToInpaint, &QAction::triggered, this, &ClickableImgLabel::onClickSendToInpaint);
	connect(actSendToUpscale, &QAction::triggered, this, &ClickableImgLabel::onClickSendToUpscale);

	ownsImg = false;
}

ClickableImgLabel::~ClickableImgLabel()
{
	if (ownsImg && originalImg)
	{
		delete originalImg;
		delete pointerToFilePath;
	}

	((QLabel*)this)->~QLabel();
}

void ClickableImgLabel::loadImage(const QString& imgPath)
{
	originalImg = new QImage(imgPath);
	pointerToFilePath = new QString(imgPath);

	ownsImg = true;
	setImage(originalImg);
}

void ClickableImgLabel::setImage(QImage* img)
{
	originalImg = img;
	setPixmap(
		QPixmap::fromImage(originalImg->scaled(pixmap().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation))
	);
}

void ClickableImgLabel::contextMenuEvent(QContextMenuEvent* event)
{
	QLabel::contextMenuEvent(event);
	if (!pointerToFilePath)
		return;

	QMenu ctxMenu(this);

	QAction action("Show in folder", this);
	connect(&action, &QAction::triggered, this, &ClickableImgLabel::showInFolder);
	ctxMenu.addAction(&action);

	ctxMenu.addAction(actSendToImg2Img);
	ctxMenu.addAction(actSendToInpaint);
	ctxMenu.addAction(actSendToUpscale);
	ctxMenu.exec(event->globalPos());
}

void ClickableImgLabel::mousePressEvent(QMouseEvent* event)
{
	QLabel::mousePressEvent(event);

	if (!originalImg)
		return;

	if (event->button() != Qt::MouseButton::LeftButton)
		return;

	if (originalImg->size().width() > 1024 && originalImg->size().height() > 1024)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(*pointerToFilePath));
		return;
	}

	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle("Image Preview");
	QVBoxLayout *layout = new QVBoxLayout(dialog);

	QLabel *imgLabel = new QLabel(dialog);
	imgLabel->setPixmap(QPixmap::fromImage(*originalImg));
	layout->addWidget(imgLabel);

	dialog->resize(originalImg->size());
	dialog->exec();
}

void ClickableImgLabel::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty())
	{
		QList<QUrl> urls = event->mimeData()->urls();
		for (const QUrl& url : urls)
		{
			QFileInfo fileInfo(url.toLocalFile());
			if (fileInfo.exists() && fileInfo.isFile() &&
				QImageReader::imageFormat(fileInfo.filePath()).isEmpty() == false) {
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void ClickableImgLabel::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		for (const QUrl& url : urls)
		{
			QString filePath = url.toLocalFile();
			QFileInfo fileInfo(filePath);
			if (fileInfo.exists() && fileInfo.isFile() && QImageReader::imageFormat(filePath).isEmpty() == false) {
				loadImage(filePath);
				event->acceptProposedAction();
				return;
			}
		}
	}
}

void ClickableImgLabel::showInFolder()
{
	if (pointerToFilePath && !pointerToFilePath->isEmpty())
	{
		QString winPath = QDir::toNativeSeparators(*pointerToFilePath);
		QStringList args;

		args << "/select," << winPath;
		QProcess::startDetached("explorer", args);
	}
}

void ClickableImgLabel::onClickSendToImg2Img()
{
	emit sendToImg2Img(originalImg);
}

void ClickableImgLabel::onClickSendToInpaint()
{
	emit sendToInpaint(originalImg);
}

void ClickableImgLabel::onClickSendToUpscale()
{
	emit sendToUpscale(originalImg);
}
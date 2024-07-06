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
	pToOriginalFilePath = nullptr;

	sendToImg2Img = new QAction("Send to Img2Img", this);
	sendToInpaint = new QAction("Send to Inpaint", this);
	sendToUpscale = new QAction("Send to Upscale", this);

	connect(sendToImg2Img, &QAction::triggered, this, &ClickableImgLabel::OnClickSendToImg2Img);
	connect(sendToInpaint, &QAction::triggered, this, &ClickableImgLabel::OnClickSendToInpaint);
	connect(sendToUpscale, &QAction::triggered, this, &ClickableImgLabel::OnClickSendToUpscale);

	ownsImg = false;
}

ClickableImgLabel::~ClickableImgLabel()
{
	if (ownsImg && originalImg)
	{
		delete originalImg;
		delete pToOriginalFilePath;
	}

	((QLabel*)this)->~QLabel();
}

void ClickableImgLabel::loadImage(const QString& imgPath)
{
	originalImg = new QImage(imgPath);
	pToOriginalFilePath = new QString(imgPath);

	ownsImg = true;
	setImage(originalImg);
}

void ClickableImgLabel::setImage(QImage* img)
{
	originalImg = img;
	setPixmap(QPixmap::fromImage(originalImg->scaled(pixmap()->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void ClickableImgLabel::contextMenuEvent(QContextMenuEvent* event)
{
	QLabel::contextMenuEvent(event);
	if (!pToOriginalFilePath)
		return;

	QMenu ctxMenu(this);

	QAction action("Show in folder", this);
	connect(&action, &QAction::triggered, this, &ClickableImgLabel::showInFolder);
	ctxMenu.addAction(&action);

	ctxMenu.addAction(sendToImg2Img);
	ctxMenu.addAction(sendToInpaint);
	ctxMenu.addAction(sendToUpscale);
	ctxMenu.exec(event->globalPos());
}

void ClickableImgLabel::mousePressEvent(QMouseEvent* event)
{
	QLabel::mousePressEvent(event);

	if (!originalImg)
		return;

	if (event->button != Qt::LeftButton)
		return;

	if (originalImg->size().width() > 1024 && originalImg->size().height() > 1024)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(*pToOriginalFielPath));
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

void ClickableImgLabel::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()
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

void ClickableImageLabel::showInFolder()
{
	if (pToOriginalFilePath && !pToOriginalFilePath->isEmpty())
	{
		QString winPath = QDir::toNativeSeparators(*pToOriginalFilePath);
		QStringList args;

		args << "/select," << winPath;
		QProcess::startDetached("explorer", args);
	}
}

void ClickableImageLabel::onClickSendToImg2Img()
{
	emit sendToImg2Img(originalImg);
}

void ClickableImageLabel::onClickSendToInpaint()
{
	emit sendToInpaint(originalImg);
}

void ClickableImageLabel::onClickSendToUpscale()
{
	emit sendToUpscale(originalImg);
}
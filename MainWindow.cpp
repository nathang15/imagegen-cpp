#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <random>
#include <QSpacerItem>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>

//#include "ESRGAN.h"

static QString createOutputFolder() {
    QString appPath = QCoreApplication::applicationDirPath();
    QString outDir = appPath + "/output";

    QDir dir(outDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return outDir;
}

static QString saveImage(const QImage& img, const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    int fileNum = 1;
    QString filePath;
    do {
        filePath = QString("%1/%2.png").arg(directoryPath).arg(fileNumber, 3, 10, QChar('0'));
        ++fileNum;
    } while (QFile::exists(filePath));

    image.save(filePath, "PNG");
    qDebug() << "Image saved to: " << filePath;
    return filePath;
}

void MainWindow::showEvent(QShowEvent* ev) {
    QMainWindow::showEvent(ev);

    if (FirstShowed)
        return;

    ParentWin->resize(1000, 500);

    FirstShowed = true;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    useFirst = true;
    previews = nullptr;

    outDir = createOutputFolder();
    curPgb = nullptr;

    curInferThread = nullptr;

    progressPoller = new QTimer(this);
    progressPoller->setInterval(100);

    connect(progressPoller, &QTimer::timeout, this, &MainWindow::onProgressPoll);

    progressPoller->start();

    processing = false;
    curItemNum = 0;
    curImgNum = 0;
    curImgDisplayIndex = 0;

    ui->scraImgPreviews->RegisterContentsWidget(ui->scrollAreaWidgetContents);
    curAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();

    updateModelListing();

    ResetViewports();

    ui->grpImg2Img->hide();

    ui->widInpaintCanvas->hide();
    ui->labelImgArrow->hide();

    ui->btnCancel->hide();
    ui->btnClearInpaint->hide();
    FirstShowed = false;
}

void MainWindow::onInpaintWidImgSet()
{
}

void MainWindow::onSendImg2Img(QImage* img)
{
}

void MainWindow::onSendToInpaint(QImage* img)
{
}

void MainWindow::onSendToUpscale(QImage* img)
{
}

void MainWindow::onLabelCtxMenu(QMenu* ctxMenu)
{
}

void MainWindow::onThreadDone()
{
}

void MainWindow::onTopBarHoverEnter(size_t lblIndex)
{
}

void MainWindow::onTopBarHoverExit(size_t lblIndex)
{
}

void MainWindow::onTopBarClick(size_t lblIndex)
{
}

void MainWindow::onGenerateBtnClicked()
{
}

void MainWindow::onLoadModelBtnClicked()
{
}

void MainWindow::onImgFwdBtnClicked()
{
}

void MainWindow::ImgBwdBtnClicked()
{
}

void MainWindow::onScrollLeft()
{
}

void MainWindow::onScrollRight()
{
}

void MainWindow::onOpenOutputsFolder()
{
}

void MainWindow::onClearCurOutput2()
{
}

void MainWindow::onCancelBtnClicked()
{
}

void MainWindow::onRefreshModelsList()
{
}

void MainWindow::onCheckImg2ImgState(int arg)
{
}

void MainWindow::onDenoiseStrengthValueSet(int val)
{
}

void MainWindow::onCheckInpaintState(int arg)
{
}

void MainWindow::onClearInpaintBtnClicked()
{
}

void MainWindow::onUpscaleBtnClicked()
{
}

void MainWindow::onLoadUpscalerBtnClicked()
{
}

int32_t MainWindow::getNeighbor(size_t inIndex)
{
    return 0;
}

void MainWindow::iterateQueue()
{
}

bool MainWindow::loadingModels()
{
    return false;
}

void MainWindow::updateModelsList() {
    QDir appDir(QCoreApplication::applicationDirPath());

    QDir modelsDir(appDir.absoluteFilePath("models"));

    if (!modelsDir.exists()) {
        loadingModels = false;
        ui->editModelPath->setEditable(!loadingModels);
        return;
    }

    modelsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    QFileInfoList folders = modelsDir.entryInfoList();

    ui->editModelPath->setCurrentText("");
    ui->editModelPath->clear();

    foreach(const QFileInfo& folder, folders) {
        ui->editModelPath->addItem(folder.fileName());
    }

    LoadingFromModelsFolder = true;
    ui->editModelPath->setEditable(!loadingModels);
}

void MainWindow::updateSelectedTopBarImg(size_t NewSelected)
{
}

void MainWindow::resetViewports() {
    const int PREVIEW_RES = 430;

    QPixmap whiteFillPixm = QPixmap(PREVIEW_RES, PREVIEW_RES);
    whiteFillPixm.fill(Qt::white);

    ui->labelLeftImg->setPixmap(WhiteFillPixm);
    ui->labelLeftImg->OriginalImage = nullptr;
    ui->labelLeftImg->pToOriginalFilePath = nullptr;

    ui->labelImg->setPixmap(WhiteFillPixm);
    ui->labelImg->OriginalImage = nullptr;
    ui->labelImg->pToOriginalFilePath = nullptr;
}

void MainWindow::onImg2ImgEnabled()
{
}

void MainWindow::updateUpscalerList()
{
}


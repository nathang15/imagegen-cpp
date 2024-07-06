#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <random>
#include <QSpacerItem>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>

#include "ESRGAN.h"

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

void MainWindow::onImgDone(QImage inImg, sdJobType jobType) {

}


void MainWindow::onInpaintWidImgSet() {
    ui->labelImg2ImgASsist->hide();
}

void MainWindow::onSendImg2Img(QImage* img) {
    ui->chkImg2Img->setChecked(true);
    ui->widInpaintCanvas->loadImage(*img);
    ui->chkInpaint->setChecked(false);
}

void MainWindow::onSendToInpaint(QImage* img) {
    ui->chkInpaint->setChecked(true);
	ui->widInpaintCanvas->loadImage(*img);
	ui->chkImg2Img->setChecked(true);
}

void MainWindow::onSendToUpscale(QImage* img) {
    ui->tabsMain->setCurrentIndex(1);
    ui->tabsUpsOptions->setCurrentIndex(0);
    ui->labelUpscalePreImg->setImage(*img);
}

void MainWindow::onLabelCtxMenu(QMenu* ctxMenu) {
}

void MainWindow::onProgressPoll() {
    if (!curPgb)
        return;
    curPgb->setValue((int_32)(curAsyncSrc->state().progress * 100.f));
}

void MainWindow::onThreadDone() {
    processing = false;
    iterateQueue();
}

void MainWindow::onTopBarHoverEnter(size_t labelIndex) {
    int32_t neighbor = getNeighbor(labelIndex);
    if (neighbor == -1)
        return;

    TopBarImages[neighbor]->setHoveringBorder(true);
}

void MainWindow::onTopBarHoverExit(size_t labelIndex) {
    int32_t neighbor = getNeighbor(labelIndex);
    if (neighbor == -1)
        return;

    TopBarImages[neighbor]->setHoveringBorder(false);
}

void MainWindow::onTopBarClick(size_t labelIndex) {

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


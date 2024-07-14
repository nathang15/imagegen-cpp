#include "MainWindow.h"
#include "Ui_MainWindow.h"
#include <random>
#include <QSpacerItem>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>
#include <QProcess>
#include "ESRGAN.h"
#include <QScrollBar>
#include <QWindow>

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
        filePath = QString("%1/%2.png").arg(dirPath).arg(fileNum, 3, 10, QChar('0'));
        ++fileNum;
    } while (QFile::exists(filePath));

    img.save(filePath, "PNG");
    qDebug() << "Image saved to: " << filePath;
    return filePath;
}

void MainWindow::showEvent(QShowEvent* ev) {
    QMainWindow::showEvent(ev);

    if (FirstShowed)
        return;

    //ParentWin->resize(1000, 500);

    FirstShowed = true;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
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

    ui->scraImgPreviews->registerContentsWidget(ui->scrollAreaWidgetContents);
    curAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();

    updateModelsList();

    resetViewports();

    ui->grpImg2Img->hide();

    ui->widInpaintCanvas->hide();
    ui->lblImgArrow->hide();

    ui->btnCancel->hide();
    ui->btnClearInpaint->hide();
    FirstShowed = false;

    connect(ui->widInpaintCanvas, &Canvas::OnImageSet, this, &MainWindow::onInpaintWidImgSet);
    connect(ui->lblImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(ui->lblImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(ui->lblImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);
    connect(ui->lblLeftImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(ui->lblLeftImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(ui->lblLeftImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);

    updateUpscalerList();
}

MainWindow::~MainWindow()
{
    onClearCurOutput2();
    curModel.Destroy();
    try
    {
        delete ui;
    }
    catch (...)
    {
        //ERROR
    }
}

std::vector<QString> jobTypeToOutDir = { "txt2img", "img2img", "upscale" };

void MainWindow::onImgDone(QImage inImg, SDJobType jobType) {
    const int PREVIEW_RES = 430;

    QString dateSubfolder = QDate::currentDate().toString("dd-MM-yyyy");
    QString folder = jobTypeToOutDir[(size_t)jobType];
    QString targetDir = QString("%1/%3/%2").arg(outDir, dateSubfolder, folder);

    QString outPath = saveImage(inImg, targetDir);

    if (jobType == SDJobType::upscale)
    {
        ui->lblUpscalePostImg->loadImage(outPath);
        return;
    }

    ++curImgNum;

    if (previews == nullptr)
    {
        ui->scraLayout->removeItem(previews);
        delete previews;
        previews = nullptr;
    }

    TopbarImg* imgPreviewTop = new TopbarImg(Q_NULLPTR);
    imgPreviewTop->setPixmap(QPixmap::fromImage(inImg).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imgPreviewTop->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum));

    topBarImgs.push_back(imgPreviewTop);

    imgPreviewTop->VecIndex = topBarImgs.size() - 1;
    imgPreviewTop->OriginalImg = inImg.copy();
    imgPreviewTop->FilePath = outPath;

    connect(imgPreviewTop, &TopbarImg::hoverEnter, this, &MainWindow::onTopBarHoverEnter);
    connect(imgPreviewTop, &TopbarImg::hoverExit, this, &MainWindow::onTopBarHoverExit);
    connect(imgPreviewTop, &TopbarImg::mouseClicked, this, &MainWindow::onTopBarClick);

    ui->scraLayout->addWidget(imgPreviewTop);

    onTopBarClick(imgPreviewTop->VecIndex);

    ui->lblAllGensProgress->setText(QString::number(taskQueue.size()) + "orders in queue\n" + "Image" + QString::number(curImgNum) + "/" + QString::number(ui->pgbAllGens->maximum()));

    ui->pgbAllGens->setValue(curImgNum);

    previews = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->scraLayout->addSpacerItem(previews);

    ui->scraImgPreviews->updateGeometry();
    imgPreviewTop->updateGeometry();

    QRect r = imgPreviewTop->geometry();
    ui->scraImgPreviews->horizontalScrollBar()->setValue(r.right());
}

void MainWindow::onInpaintWidImgSet() {
    ui->lblImg2ImgAssist->hide();
}

void MainWindow::onSendImg2Img(QImage* img) {
    ui->chkImg2Img->setChecked(true);
    ui->widInpaintCanvas->loadImg(*img);
    ui->chkInpaint->setChecked(false);
}

void MainWindow::onSendToInpaint(QImage* img) {
    ui->chkInpaint->setChecked(true);
    ui->widInpaintCanvas->loadImg(*img);
    ui->chkImg2Img->setChecked(true);
}

void MainWindow::onSendToUpscale(QImage* img) {
    ui->tabsMain->setCurrentIndex(1);
    ui->tabsUpsOptions->setCurrentIndex(0);
    ui->lblUpscalePreImage->setImage(img);
}

void MainWindow::onLabelCtxMenu(QMenu* ctxMenu) {
}

void MainWindow::onProgressPoll() {
    if (!curPgb)
        return;
    curPgb->setValue((int32_t)(curAsyncSrc->state().progress * 100.f));
}

void MainWindow::onThreadDone() {
    processing = false;
    iterateQueue();
}

void MainWindow::onTopBarHoverEnter(size_t labelIndex) {
    int32_t neighbor = getNeighbor(labelIndex);
    if (neighbor == -1)
        return;

    topBarImgs[neighbor]->setHoveringBorder(true);
}

void MainWindow::onTopBarHoverExit(size_t labelIndex) {
    int32_t neighbor = getNeighbor(labelIndex);
    if (neighbor == -1)
        return;

    topBarImgs[neighbor]->setHoveringBorder(false);
}

void MainWindow::onTopBarClick(size_t labelIndex) {
    if (!topBarImgs.size())
        return;

    const int PREVIEW_RES = 430;

    QPixmap viewPortImg = QPixmap::fromImage(topBarImgs[labelIndex]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int32_t neighbor = getNeighbor(labelIndex);

    if (neighbor == -1)
    {
        ClickableImgLabel* viewPort = ui->lblLeftImg;

        if (!useFirst)
            viewPort = ui->lblImg;

        viewPort->setPixmap(viewPortImg);
        viewPort->originalImg = &topBarImgs[labelIndex]->OriginalImg;
        viewPort->pointerToFilePath = &topBarImgs[labelIndex]->FilePath;

        useFirst = !useFirst;
    }
    else
    {
        QPixmap secondViewportImg = QPixmap::fromImage(topBarImgs[neighbor]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        ui->lblLeftImg->setPixmap(viewPortImg);
        ui->lblLeftImg->originalImg = &topBarImgs[labelIndex]->OriginalImg;
        ui->lblLeftImg->pointerToFilePath = &topBarImgs[labelIndex]->FilePath;

        ui->lblImg->setPixmap(secondViewportImg);
        ui->lblImg->originalImg = &topBarImgs[neighbor]->OriginalImg;
        ui->lblImg->pointerToFilePath = &topBarImgs[neighbor]->FilePath;

        useFirst = true;

    }
    updateSelectedTopBarImg(labelIndex);
    curImgDisplayIndex = labelIndex;
}

std::vector<Axodox::MachineLearning::StableDiffusionSchedulerKind> ComboBoxIDToScheduler =
{
    Axodox::MachineLearning::StableDiffusionSchedulerKind::DpmPlusPlus2M,
    Axodox::MachineLearning::StableDiffusionSchedulerKind::EulerAncestral
};

void MainWindow::onGenerateBtnClicked()
{
    if (!curModel.IsLoaded())
        onLoadModelBtnClicked();

    Axodox::MachineLearning::StableDiffusionOptions options;
    options.BatchSize = ui->spbBatchSize->value();
    options.GuidanceScale = (float)ui->spbCFGScale->value();

    QString resolution = ui->edtResolution->text();
    QStringList widthHeight = resolution.split("x");

    options.Height = widthHeight[1].toInt();
    options.Width = widthHeight[0].toInt();

    //options.Scheduler = Axodox::MachineLearning::StableDiffusionSchedulerKind;
    options.StepCount = ui->spbSamplingSteps->value();

    if (ui->chkImg2Img->isChecked())
    {
        float BaseDenStrength = (float)ui->sliDenoiseStrength->value();
        options.DenoisingStrength = BaseDenStrength / 100.f;
    }

    if (ui->edtSeed->text().isEmpty())
        options.Seed = UINT32_MAX;
    else
        options.Seed = ui->edtSeed->text().toUInt();

    options.Scheduler = ComboBoxIDToScheduler[ui->cbSampler->currentIndex()];

    sdOrder Ord{
        ui->edtPrompt->toPlainText().toStdString(),
        ui->edtNegPrompt->toPlainText().toStdString(),
        options,
        (uint32_t)ui->spbBatchCount->value(),
        ui->edtSeed->text().isEmpty()
    };

    if (ui->chkImg2Img->isChecked())
        Ord.inImage = ui->widInpaintCanvas->getImg().copy();
    if (ui->chkInpaint->isChecked())
        Ord.inMask = ui->widInpaintCanvas->getMask().copy();

    taskQueue.push(Ord);

    ui->lblAllGensProgress->setText(
        QString::number(taskQueue.size() + 1) + " orders in queue\n" +
        "Image " + QString::number(curImgNum) + "/" + QString::number(ui->pgbAllGens->maximum())
    );

    iterateQueue();
}

void MainWindow::onLoadModelBtnClicked()
{
    QString appDirPath = QCoreApplication::applicationDirPath();

    QString fullModelPath = appDirPath + "/models/" + ui->edtModelPath->currentText().trimmed();

    if (!loadingModels)
    {
        fullModelPath = ui->edtModelPath->currentText().trimmed();
    }

    curModel.Load(fullModelPath.toStdString());
}

void MainWindow::onImgFwdBtnClicked()
{
    size_t maxIndex = topBarImgs.size() - 1;

    size_t doubleHopIndex = curImgDisplayIndex + 2;
    size_t singleHopIndex = curImgDisplayIndex + 1;

    if (doubleHopIndex < maxIndex)
        onTopBarClick(doubleHopIndex);
    else if (singleHopIndex < maxIndex)
        onTopBarClick(singleHopIndex);
    else
        return;
}

void MainWindow::onImgBwdBtnClicked()
{
    size_t MinIndex = 0;

    int32_t DoubleHopIndex = curImgDisplayIndex - 2;

    DoubleHopIndex = std::max<int32_t>(DoubleHopIndex, 0);

    onTopBarClick(DoubleHopIndex);
}

void MainWindow::onScrollLeft()
{
    onImgBwdBtnClicked();
}

void MainWindow::onScrollRight()
{
    onImgFwdBtnClicked();
}

void MainWindow::onOpenOutputsFolder()  
{
    QString winPath = QDir::toNativeSeparators(outDir);

    QStringList args;
    args << winPath;
    QProcess::startDetached("explorer", args);
}

void MainWindow::onClearCurOutput2()
{
    if (previews)
    {
        ui->scraLayout->removeItem(previews);
        delete previews;
        previews = nullptr;
    }

    for (auto* imgPreview : topBarImgs)
    {
        ui->scraLayout->removeWidget(imgPreview);
        delete imgPreview;
    }

    topBarImgs.clear();
    resetViewports();

    curImgDisplayIndex = 0;
}

void MainWindow::onCancelBtnClicked()
{
    curAsyncSrc->cancel();
}

void MainWindow::onRefreshModelsList()
{
    updateModelsList();
}

void MainWindow::onCheckImg2ImgState(int arg)
{
    bool enabled = (bool)arg;

    ui->grpImg2Img->setVisible(enabled);
    ui->widInpaintCanvas->setVisible(enabled);
    ui->lblImgArrow->setVisible(enabled);
    ui->btnClearInpaint->setVisible(enabled);

    if (enabled)
    {
        ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
        onImg2ImgEnabled();
    }
    else
    {
        ui->horizontalSpacer->changeSize(40, 20, QSizePolicy::Expanding);
        ui->horizontalSpacer_2->changeSize(40, 20, QSizePolicy::Expanding);
    }

    update();

}

void MainWindow::onDenoiseStrengthValueSet(int val)
{
    ui->lblDenoisePercShow->setText(QString::number(val) + "%");
}

void MainWindow::onCheckInpaintState(int arg)
{
    ui->widInpaintCanvas->setVisible((bool)arg);
}

void MainWindow::onClearInpaintBtnClicked()
{
    ui->widInpaintCanvas->clearStrokes();
}

void MainWindow::onUpscaleBtnClicked()
{
    if (!curUpscaler.IsLoaded())
        onLoadUpscalerBtnClicked();

    sdOrder Ord{
        ui->edtPrompt->toPlainText().toStdString(),
        ui->edtNegPrompt->toPlainText().toStdString(),
        Axodox::MachineLearning::StableDiffusionOptions{},
        (uint32_t)ui->spbBatchCount->value(),
        ui->edtSeed->text().isEmpty()
    };

    Ord.inImage = *ui->lblUpscalePreImage->originalImg;
    Ord.isUpscale = true;

    taskQueue.push(Ord);
    iterateQueue();
}

void MainWindow::onLoadUpscalerBtnClicked()
{
    if (!curModel.GetEnv())
        curModel.LoadMinimal();
    curUpscaler.SetEnv(curModel.GetEnv());

    curUpscaler.Load(
        QString(QCoreApplication::applicationDirPath() + "/upscalers" + ui->cbUpscalerModels->currentText() + "onnx").toStdString()
    );
}

int32_t MainWindow::getNeighbor(size_t inIndex)
{
    size_t maxIndex = topBarImgs.size() - 1;

    size_t attemptIndex = inIndex + 1;

    if (attemptIndex > maxIndex)
        return -1;

    return (int32_t)attemptIndex;
}


void MainWindow::iterateQueue()
{
    if (taskQueue.size() && !processing)
        ui->btnCancel->hide();
    else
        ui->btnCancel->show();

    if (processing)
        return;

    if (!taskQueue.size())
    {
        curItemNum = 0;
        return;
    }

    sdOrder curOrd = taskQueue.front();
    Inferer* inferThread = new Inferer;

    if (curAsyncSrc->is_cancelled())
        curAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();

    inferThread->options = curOrd.options;
    inferThread->model = &curModel;
    inferThread->prompt = curOrd.prompt;
    inferThread->negPrompt = curOrd.negPrompt;
    inferThread->batchCount = curOrd.batchCount;
    inferThread->randSeed = curOrd.randSeed;

    if (!curOrd.inImage.isNull())
        inferThread->inImg = curOrd.inImage.copy();

    if (!curOrd.inMask.isNull())
        inferThread->inMask = curOrd.inMask.copy();

    if (curOrd.isUpscale)
    {
        curUpscaler.SetEnv(curModel.GetEnv());
        inferThread->esrgan = &curUpscaler;
        curPgb = ui->pgbUpscaleProg;
    }
    else
    {
        inferThread->esrgan = nullptr;
        curPgb = ui->pgbCurrentGen;
    }

    connect(inferThread, &Inferer::done, this, &MainWindow::onImgDone);
    connect(inferThread, &Inferer::threadDone, this, &MainWindow::onThreadDone);

    connect(inferThread, &Inferer::done, inferThread, &QObject::deleteLater);

    curInferThread = inferThread;
    ui->pgbCurrentGen->setRange(0, 100);
    ui->pgbUpscaleProg->setRange(0, 100);

    inferThread->asyncSrc = curAsyncSrc.get();
    inferThread->start();

    ++curItemNum;

    ui->pgbAllGens->setRange(0, curOrd.batchCount);

    taskQueue.pop();
    processing = true;

    ui->lblAllGensProgress->setText(
        QString::number(taskQueue.size()) + " orders in queue\n" +
        "Image 0/" + QString::number(curImgNum) + "/" + QString::number(curOrd.batchCount)
    );

    curImgNum = 0;
}

void MainWindow::updateModelsList() {
    QDir appDir(QCoreApplication::applicationDirPath());

    QDir modelsDir(appDir.absoluteFilePath("models"));

    if (!modelsDir.exists()) {
        loadingModels = false;
        ui->edtModelPath->setEditable(!loadingModels);
        return;
    }

    modelsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    QFileInfoList folders = modelsDir.entryInfoList();

    ui->edtModelPath->setCurrentText("");
    ui->edtModelPath->clear();

    foreach(const QFileInfo & folder, folders) {
        ui->edtModelPath->addItem(folder.fileName());
    }

    loadingModels = true;
    ui->edtModelPath->setEditable(!loadingModels);
}

void MainWindow::updateSelectedTopBarImg(size_t NewSelected)
{
    for (auto*& imgWid : topBarImgs)
    {
        imgWid->setSelectedBorder(false);
    }

    topBarImgs[NewSelected]->setSelectedBorder(true);

    int32_t neighbor = getNeighbor(NewSelected);

    if (neighbor != -1)
        topBarImgs[neighbor]->setSelectedBorder(true);
}

void MainWindow::resetViewports() {
    const int PREVIEW_RES = 430;

    QPixmap whiteFillPixm = QPixmap(PREVIEW_RES, PREVIEW_RES);
    whiteFillPixm.fill(Qt::white);

    ui->lblLeftImg->setPixmap(whiteFillPixm);
    ui->lblLeftImg->originalImg = nullptr;
    ui->lblLeftImg->pointerToFilePath = nullptr;

    ui->lblImg->setPixmap(whiteFillPixm);
    ui->lblImg->originalImg = nullptr;
    ui->lblImg->pointerToFilePath = nullptr;
}

void MainWindow::onImg2ImgEnabled()
{
    const int PREVIEW_RES = 430;
    QImage white(PREVIEW_RES, PREVIEW_RES, QImage::Format_RGBA8888);
    white.fill(Qt::white);
    ui->widInpaintCanvas->loadImg(white);
}

void MainWindow::updateUpscalerList()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    QDir upscalerDir(appDir.absoluteFilePath("upscalers"));

    if (!upscalerDir.exists())
    {
        ui->cbUpscalerModels->setDisabled(true);
        return;
    }

    upscalerDir.setNameFilters(QStringList() << "*.onnx");
    upscalerDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList files = upscalerDir.entryInfoList();

    ui->cbUpscalerModels->setCurrentText("");
    ui->cbUpscalerModels->clear();

    foreach(const QFileInfo & file, files)
    {
        QString modelName = file.baseName();
        ui->cbUpscalerModels->addItem(modelName);
    }

    ui->cbUpscalerModels->setDisabled(false);

    if (!files.isEmpty())
        ui->cbUpscalerModels->setCurrentIndex(0);
}

#include "MainWindow.h"
#include "Ui_MainWindow.h"
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

    updateModelsList();

    resetViewports();

    ui->grpImg2Img->hide();

    ui->widInpaintCanvas->hide();
    ui->labelImgArrow->hide();

    ui->btnCancel->hide();
    ui->btnClearInpaint->hide();
    FirstShowed = false;

    connect(ui->widInpaintCanvas, &Canvas::OnImageSet, this, &MainWindow::onInpaintWidImgSet);
    connect(ui->labelImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(ui->labelImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(ui->labelImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);
    connect(ui->labelLeftImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(ui->labelLeftImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(ui->labelLeftImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);

    updateUpscalerList();
}

MainWindow::~MainWindow()
{
    onClearCurOutput2();
    curModel.destroy();
    try
    {
        delete Ui;
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
    QString targetDir = QString("%1/%3/%2".arg(outDir, dateSubfolder, folder));

    QString outPath = saveImage(inImg, targetDir);

    if (jobType == SDJobType::upscale)
    {
        ui->labelUpscalePostImg->loadImg(outPath);
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

    TopbarImg.push_back(imgPreviewTop);

    imgPreviewTop->VecIndex = TopbarImg.size() - 1;
    imgPreviewTop->OriginalImg = inImg.copy();
    imgPreviewTop->FilePath = outPath;

    connect(imgPreviewTop, &TopbarImg::hoverEnter, this, &MainWindow::onTopBarHoverEnter);
    connect(imgPreviewTop, &TopbarImg::hoverExit, this, &MainWindow::onTopBarHoverExit);
    connect(imgPreviewTop, &TopbarImg::mouseClicked, this, &MainWindow::onTopBarClick);

    ui->scraLayout->addWidget(imgPreviewTop);

    onTopBarClick(imgPreviewTop->VecIndex);

    ui->labelAllGensProgress->setText(QString::number(taskQueue.size()) + "orders in queue\n" + "Image" + QString::number(curImgNum) + "/" + QString::number(ui->pgbAllGens->maximum()));

    ui->pgbAllGens->setValue(curImgNum);

    previews = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->scraLayout->addSpacerItem(previews);

    ui->scraImgPreviews->updateGeometry();
    imgPreviewTop->updateGeometry();

    QRect r = imgPreviewTop->geometry();
    ui->scraImgPreviews->horizontalScrollbar()->setValue(r.right());
}

void MainWindow::onInpaintWidImgSet() {
    ui->labelImg2ImgAssist->hide();
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
        ClickableImgLabel* viewPort = ui->labelLeftImg;

        if (!useFirst)
			viewPort = ui->labelImg;

        viewPort->setPixmap(viewPortImg);
        viewPort->OriginalImg = &topBarImgs[labelIndex]->OriginalImg;
        viewPort->pToOriginalFilePath = &topBarImgs[labelIndex]->FilePath;

        useFirst = !useFirst;
    }
    else
    {
        QPixmap secondViewportImg = QPixmap::fromImage(topBarImgs[neighbor]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        ui->labelLeftImg->setPixmap(viewPortImg);
        ui->labelLeftImg->OriginalImg = &topBarImgs[labelIndex]->OriginalImg;
        ui->labelLeftImg->pToOriginalFilePath = &topBarImgs[labelIndex]->FilePath;

        ui->labelImg->setPixmap(secondViewportImg);
        ui->labelImg->originalImage = &topBarImgs[neighbor]->OriginalImg;
        ui->labelImg->pToOriginalFilePath = &topBarImgs[neighbor]->FilePath;

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
    if (!curModel.loaded())
        onLoadModelBtnClicked();

    Axodox::MachineLearning::StableDiffusionOptions options;
    options.BatchSize = ui->spbBatchSize->value();
    options.GuidanceScale = (float)ui->spbCFGScale->value();

    QString resolution = ui->editResolution->text();
    QStringList widthHeight = resolution.split("x");

    options.Height = widthHeight[1].toInt();
    options.Width = widthHeight[0].toInt();

    options.PredictionType = Axodox::MachineLearning::StableDiffusionSchedulerPredictionType::V;
    options.StepCount = ui->spbSamplingSteps->value();

    if (ui->chkImg2Img->checked())
    {
        float BaseDenStrength = (float)ui->sliDenoiseStrength->value();
        options.DenoisingStrength = BaseDenStrength / 100.f;
    }

    if (ui->editSeed->text().isEmpty())
        options.Seed = UINT32_MAX;
    else
        options.Seed = ui->editSeed->text().toUInt();

    options.Scheduler = ComboBoxIDToScheduler[ui->cbSampler->currentIndex()];

    sdOrder Ord{ 
        ui->editPrompt->toPlainText().toStdString(), 
        ui->editNegPrompt->toPlainText().toStdString(), 
        options, 
        (uint32_t)ui->spbBatchCount->value(), 
        ui->editSeed->text().isEmpty() 
    };

    if (ui->chkImg2Img->checked())
        Ord.inImage = ui->widInpaintCanvas->getImage().copy();
    if (ui->chkInpaint->checked())
        Ord.inMask = ui->widInpaintCanvas->getMask().copy();

    taskQueue.push(Ord);

    ui->labelAllGensProgress->setText(
        QString::number(taskQueue.size() + 1) + " orders in queue\n" +
        "Image " + QString::number(curImgNum) + "/" + QString::number(ui->pgbAllGens->maximum())
    );

    iterateQueue();
}

void MainWindow::onLoadModelBtnClicked()
{
    QString appDirPath = QCoreApplication::applicationDirPath();

    QString fullModelPath = appDirPath + "/models/" + ui->editModelPath->currentText().trimmed();

    if (!loadingModels)
	{
		fullModelPath = ui->editModelPath->currentText().trimmed();
	}

    curModel.load(fullModelPath.toStdString());
}

void MainWindow::onImgFwdBtnClicked()
{
    size_t maxIndex = topBarImgs.size() - 1;

    size_t doubleHopIndex = curImgDisplayIndex + 2;
    size_t singleHopIndex = curImgDisplayIndex + 1;

    if (doubleHopeIndex < maxIndex)
        onTopBarClick(doubleHopIndex);
    else if (singleHopIndex < maxIndex)
        onTopBarClick(singleHopIndex);
    else
        return;
}

void MainWindow::onImgBwdBtnClicked()
{
    size_t minIndex = 0;

    int32_t doubleHopIndex = curImgDisplayIndex - 2;

    doubleHopIndex = std::max<int32_t>(doubleHopIndex, 0);

    onTopBarClick(doubleHopIndex);
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
    ui->labelImgArrow->setVisible(enabled);
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
    ui->labelDenoiseStrength->setText(QString::number(val) + "%");
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
    if (!curUpscaler.loaded())
        onLoadUpscalerBtnClicked();

    sdOrder Ord{
        ui->editPrompt->toPlainText().toStdString(),
        ui->editNegPrompt->toPlainText().toStdString(),
        Axodox::MachineLearning::StableDiffusionOptions{},
        (uint32_t)ui->spbBatchCount->value(),
        ui->editSeed->text().isEmpty()
    }

    Ord.inImage = *ui->labelUpscalePreImg->originalImage;
    Ord.isUpscale = true;

    taskQueue.push(Ord);
    iterateQueue();
}

void MainWindow::onLoadUpscalerBtnClicked()
{
    if (!curUpscaler.getEnv())
        curModel.loadMinimal();
    curUpscaler.setEnv(curModel.getEnv());

    curUpscaler.load(
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
        curUpscaler.setEnv(curModel.getEnv());
        inferThread->esrgan = &curUpscaler;
        curPgb = ui->pgbUpscaleProg;
    }
    else
    {
        inferThread->esrgan = nullptr;
        curPgb = ui->pgbCurGen;
    }

    connect(inferThread, &Inferer::done, this, &MainWindow::onImgDone);
    connect(inferThread, &Inferer::threadDone, this, &MainWindow::onThreadDone);

    connect(inferThread, &Inferer::done, inferThread, &QObject::deleteLater);

    curInferThread = inferThread;
    ui->pgbCurGen->setRange(0, 100);
    ui->pgbUpscaleProg->setRange(0, 100);

    inferThread->asyncSrc = curAsyncSrc.get();
    inferThread->start();

    ++curItemNum;

    ui->pgbAllGens->setRange(0, curOrd.batchCount);

    taskQueue.pop();
    processing = true;

    ui->labelAllGensProgress->setText(
		QString::number(taskQueue.size()) + " orders in queue\n" +
		"Image 0/" + QString::number(curImgNum) + "/" + QString::number(curOrd.batchCount)
	);

    curImgNum = 0;
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

    loadingModels = true;
    ui->editModelPath->setEditable(!loadingModels);
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

    ui->labelLeftImg->setPixmap(WhiteFillPixm);
    ui->labelLeftImg->originalImage = nullptr;
    ui->labelLeftImg->pToOriginalFilePath = nullptr;

    ui->labelImg->setPixmap(WhiteFillPixm);
    ui->labelImg->originalImage = nullptr;
    ui->labelImg->pToOriginalFilePath = nullptr;
}

void MainWindow::onImg2ImgEnabled()
{
    const int PREVIEW_RES = 430;
    QImage white(PREVIEW_RES, PREVIEW_RES, QImage::Format_RGBA8888);
    white.fill(Qt::white);
    ui->widInpaintCanvas->loadImage(white);
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


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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), Ui(new Ui::MainWindow){
    Ui->setupUi(this);
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

    Ui->scraImgPreviews->RegisterContentsWidget(Ui->scrollAreaWidgetContents);
    curAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();

    updateModelsList();

    resetViewports();

    Ui->grpImg2Img->hide();

    Ui->widInpaintCanvas->hide();
    Ui->labelImgArrow->hide();

    Ui->btnCancel->hide();
    Ui->btnClearInpaint->hide();
    FirstShowed = false;

    connect(Ui->widInpaintCanvas, &Canvas::OnImageSet, this, &MainWindow::onInpaintWidImgSet);
    connect(Ui->labelImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(Ui->labelImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(Ui->labelImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);
    connect(Ui->labelLeftImg, &ClickableImgLabel::sendToInpaint, this, &MainWindow::onSendToInpaint);
    connect(Ui->labelLeftImg, &ClickableImgLabel::sendToImg2Img, this, &MainWindow::onSendImg2Img);
    connect(Ui->labelLeftImg, &ClickableImgLabel::sendToUpscale, this, &MainWindow::onSendToUpscale);

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

void MainWindow::onImgDone(QImage inImg, sdJobType jobType) {
    const int PREVIEW_RES = 430;

    QString dateSubfolder = QDate::currentDate().toString("dd-MM-yyyy");
    QString folder = jobTypeToOutDir[(size_t)jobType];
    QString targetDir = QString("%1/%3/%2".arg(outDir, dateSubfolder, folder));

    QString outPath = saveImage(inImg, targetDir);

    if (jobType == SDJobType::upscale)
    {
        Ui->labelUpscalePostImg->loadImg(outPath);
        return;
    }

    ++curImgNum;

    if (previews == nullptr)
    {
        Ui->scraLayout->removeItem(previews);
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

    Ui->scraLayout->addWidget(imgPreviewTop);

    onTopBarClick(imgPreviewTop->VecIndex);

    Ui->labelAllGensProgress->setText(QString::number(taskQueue.size()) + "orders in queue\n" + "Image" + QString::number(curImgNum) + "/" + QString::number(Ui->pgbAllGens->maximum()));

    Ui->pgbAllGens->setValue(curImgNum);

    previews = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    Ui->scraLayout->addSpacerItem(previews);

    Ui->scraImgPreviews->updateGeometry();
    imgPreviewTop->updateGeometry();

    QRect r = imgPreviewTop->geometry();
    Ui->scraImgPreviews->horizontalScrollbar()->setValue(r.right());
}

void MainWindow::onInpaintWidImgSet() {
    Ui->labelImg2ImgAssist->hide();
}

void MainWindow::onSendImg2Img(QImage* img) {
    Ui->chkImg2Img->setChecked(true);
    Ui->widInpaintCanvas->loadImage(*img);
    Ui->chkInpaint->setChecked(false);
}

void MainWindow::onSendToInpaint(QImage* img) {
    Ui->chkInpaint->setChecked(true);
	Ui->widInpaintCanvas->loadImage(*img);
	Ui->chkImg2Img->setChecked(true);
}

void MainWindow::onSendToUpscale(QImage* img) {
    Ui->tabsMain->setCurrentIndex(1);
    Ui->tabsUpsOptions->setCurrentIndex(0);
    Ui->labelUpscalePreImg->setImage(*img);
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
        ClickableImgLabel* viewPort = Ui->labelLeftImg;

        if (!useFirst)
			viewPort = Ui->labelImg;

        viewPort->setPixmap(viewPortImg);
        viewPort->OriginalImg = &topBarImgs[labelIndex]->OriginalImg;
        viewPort->pToOriginalFilePath = &topBarImgs[labelIndex]->FilePath;

        useFirst = !useFirst;
    }
    else
    {
        QPixmap secondViewportImg = QPixmap::fromImage(topBarImgs[neighbor]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        Ui->labelLeftImg->setPixmap(viewPortImg);
        Ui->labelLeftImg->OriginalImg = &topBarImgs[labelIndex]->OriginalImg;
        Ui->labelLeftImg->pToOriginalFilePath = &topBarImgs[labelIndex]->FilePath;

        Ui->labelImg->setPixmap(secondViewportImg);
        Ui->labelImg->OriginalImage = &topBarImgs[neighbor]->OriginalImg;
        Ui->labelImg->pToOriginalFilePath = &topBarImgs[neighbor]->FilePath;

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
    options.BatchSize = Ui->spbBatchSize->value();
    options.GuidanceScale = (float)Ui->spbCFGScale->value();

    QString resolution = Ui->editResolution->text();
    QStringList widthHeight = resolution.split("x");

    options.Height = widthHeight[1].toInt();
    options.Width = widthHeight[0].toInt();

    options.PredictionType = Axodox::MachineLearning::StableDiffusionSchedulerPredictionType::V;
    options.StepCount = Ui->spbSamplingSteps->value();

    if (Ui->chkImg2Img->checked())
    {
        float BaseDenStrength = (float)Ui->sliDenoiseStrength->value();
        options.DenoisingStrength = BaseDenStrength / 100.f;
    }

    if (Ui->editSeed->text().isEmpty())
        options.Seed = UINT32_MAX;
    else
        options.Seed = ui->editSeed->text().toUInt();

    options.Scheduler = ComboBoxIDToScheduler[Ui->cbSampler->currentIndex()];

    sdOrder Ord{ 
        Ui->editPrompt->toPlainText().toStdString(), 
        Ui->editNegPrompt->toPlainText().toStdString(), 
        options, 
        (uint32_t)Ui->spbBatchCount->value(), 
        Ui->editSeed->text().isEmpty() 
    };

    if (Ui->chkImg2Img->checked())
        Ord.inImage = Ui->widInpaintCanvas->getImage().copy();
    if (Ui->chkInpaint->checked())
        Ord.inMask = Ui->widInpaintCanvas->getMask().copy();

    taskQueue.push(Ord);

    Ui->labelAllGensProgress->setText(
        QString::number(taskQueue.size() + 1) + " orders in queue\n" +
        "Image " + QString::number(curImgNum) + "/" + QString::number(ui->pgbAllGens->maximum())
    );

    iterateQueue();
}

void MainWindow::onLoadModelBtnClicked()
{
    QString appDirPath = QCoreApplication::applicationDirPath();

    QString fullModelPath = appDirPath + "/models/" + Ui->editModelPath->currentText().trimmed();

    if (!loadingModels)
	{
		fullModelPath = Ui->editModelPath->currentText().trimmed();
	}

    curModel.load(fullModelPath.toStdString());
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
    size_t maxIndex = topBarImgs.size() - 1;

    size_t attemptIndex = inIndex + 1;

    if (attemptIndex > maxIndex)
        return -1;

    return (int32_t)attemptIndex;
}

// TODO: IMPL

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
        Ui->editModelPath->setEditable(!loadingModels);
        return;
    }

    modelsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    QFileInfoList folders = modelsDir.entryInfoList();

    Ui->editModelPath->setCurrentText("");
    Ui->editModelPath->clear();

    foreach(const QFileInfo& folder, folders) {
        Ui->editModelPath->addItem(folder.fileName());
    }

    LoadingFromModelsFolder = true;
    Ui->editModelPath->setEditable(!loadingModels);
}

void MainWindow::updateSelectedTopBarImg(size_t NewSelected)
{
}

void MainWindow::resetViewports() {
    const int PREVIEW_RES = 430;

    QPixmap whiteFillPixm = QPixmap(PREVIEW_RES, PREVIEW_RES);
    whiteFillPixm.fill(Qt::white);

    Ui->labelLeftImg->setPixmap(WhiteFillPixm);
    Ui->labelLeftImg->OriginalImage = nullptr;
    Ui->labelLeftImg->pToOriginalFilePath = nullptr;

    Ui->labelImg->setPixmap(WhiteFillPixm);
    Ui->labelImg->OriginalImage = nullptr;
    Ui->labelImg->pToOriginalFilePath = nullptr;
}

void MainWindow::onImg2ImgEnabled()
{
}

void MainWindow::updateUpscalerList()
{
}


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <random>
#include <QSpacerItem>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>
#include <QScrollBar>
#include <QProcess>

#include "ESRGAN.h"

QString createOutputsFolder() {
    QString applicationPath = QCoreApplication::applicationDirPath();
    QString OutputsDir = applicationPath + "/outputs";

    QDir dir(OutputsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return OutputsDir;
}

QString saveImage(const QImage& image, const QString& directoryPath) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    int fileNumber = 1;
    QString filePath;
    do {
        filePath = QString("%1/%2.png").arg(directoryPath).arg(fileNumber, 3, 10, QChar('0'));
        ++fileNumber;
    } while (QFile::exists(filePath));

    image.save(filePath, "PNG");
    qDebug() << "Image saved to: " << filePath;
    return filePath;
}

void MainWindow::showEvent(QShowEvent* event)
{

    QMainWindow::showEvent(event);

    if (DidFirstShowStuff)
        return;

    DidFirstShowStuff = true;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    UseFirst = true;
    PreviewsSpacer = nullptr;

    OutpsDir = createOutputsFolder();
    CurrentPgb = nullptr;

    CurrentInferThrd = nullptr;

    progressPoller = new QTimer(this);
    progressPoller->setInterval(100);

    connect(progressPoller, &QTimer::timeout, this, &MainWindow::OnProgressPoll);

    progressPoller->start();

    IsProcessing = false;
    CurrentItemNumber = 0;
    CurrentImageNumber = 0;
    CurrentImgDisplayIndex = 0;

    ui->scraImgPreviews->RegisterContentsWidget(ui->scrollAreaWidgetContents);
    CurrentAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();


    UpdateModelListing();

    ResetViewports();

    ui->grpImg2Img->hide();

    ui->widInpaintCanvas->hide();
    ui->lblImgArrow->hide();

    ui->btnCancel->hide();
    ui->btnClearInpaint->hide();
    DidFirstShowStuff = false;

    connect(ui->widInpaintCanvas, &PaintableCanvas::OnImageSet, this, &MainWindow::OnInpaintWidImageSet);

    connect(ui->lblImg, &ClickableImageLabel::SendImageToInpaint, this, &MainWindow::OnImageSendToInpaint);
    connect(ui->lblImg, &ClickableImageLabel::SendImageToImg2Img, this, &MainWindow::OnImageSendToImg2Img);
    connect(ui->lblImg, &ClickableImageLabel::SendImageToUpscale, this, &MainWindow::OnImageSendToUpscale);


    connect(ui->lblLeftImg, &ClickableImageLabel::SendImageToInpaint, this, &MainWindow::OnImageSendToInpaint);
    connect(ui->lblLeftImg, &ClickableImageLabel::SendImageToImg2Img, this, &MainWindow::OnImageSendToImg2Img);
    connect(ui->lblLeftImg, &ClickableImageLabel::SendImageToUpscale, this, &MainWindow::OnImageSendToUpscale);

    UpdateUpscalerListing();
}

MainWindow::~MainWindow()
{
    on_actionClear_current_outputs_2_triggered();
    CurrentMdl.Destroy();

    try {
        delete ui;

    }
    catch (...) {
        // ...
    }
}

std::vector<QString> JobTypeToOutdir = { "txt2img", "img2img", "upscale" };

void MainWindow::OnImageDone(QImage InImg, StableDiffusionJobType JobType)
{
    const int PREVIEW_RES = 430;

    QString dateSubfolder = QDate::currentDate().toString("dd-MM-yyyy");
    QString ImmediateFolder = JobTypeToOutdir[(size_t)JobType];
    QString targetDirectory = QString("%1/%3/%2").arg(OutpsDir, dateSubfolder, ImmediateFolder);

    QString OutPath = saveImage(InImg, targetDirectory);

    if (JobType == StableDiffusionJobType::Upscale)
    {
        ui->lblUpscalePostImg->loadImage(OutPath);
        return;
    }

    ++CurrentImageNumber;

    if (PreviewsSpacer) {
        ui->scraLayout->removeItem(PreviewsSpacer);
        delete PreviewsSpacer;
        PreviewsSpacer = nullptr;
    }

    TopBarImg* ImgPreviewTop = new TopBarImg(Q_NULLPTR);
    ImgPreviewTop->setPixmap(QPixmap::fromImage(InImg).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ImgPreviewTop->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum));


    TopBarImages.push_back(ImgPreviewTop);

    ImgPreviewTop->VecIndex = TopBarImages.size() - 1;
    ImgPreviewTop->OriginalImg = InImg.copy();
    ImgPreviewTop->FilePath = OutPath;

    connect(ImgPreviewTop, &TopBarImg::HoverEnter, this, &MainWindow::OnTopBarHoverEnter);
    connect(ImgPreviewTop, &TopBarImg::HoverExit, this, &MainWindow::OnTopBarHoverExit);
    connect(ImgPreviewTop, &TopBarImg::MouseClicked, this, &MainWindow::OnTopBarClick);

    ui->scraLayout->addWidget(ImgPreviewTop);
    OnTopBarClick(ImgPreviewTop->VecIndex);

    ui->lblAllGensProgress->setText(
        QString::number(TaskQueue.size()) + " orders in queue\n" +
        "Image " + QString::number(CurrentImageNumber) + "/" + QString::number(ui->pgbAllGens->maximum())
    );

    ui->pgbAllGens->setValue(CurrentImageNumber);

    PreviewsSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->scraLayout->addSpacerItem(PreviewsSpacer);

    ui->scraImgPreviews->updateGeometry();

    ImgPreviewTop->updateGeometry();

    QRect r = ImgPreviewTop->geometry(); ui->scraImgPreviews->horizontalScrollBar()->setValue(r.right());

}

void MainWindow::OnInpaintWidImageSet()
{
    ui->lblImg2ImgAssist->hide();
}

void MainWindow::OnImageSendToImg2Img(QImage* SndImg)
{
    ui->chkImg2Img->setChecked(true);
    ui->widInpaintCanvas->loadImage(*SndImg);
    ui->chkInpaint->setChecked(false);
}

void MainWindow::OnImageSendToInpaint(QImage* SndImg)
{
    ui->chkImg2Img->setChecked(true);
    ui->widInpaintCanvas->loadImage(*SndImg);
    ui->chkInpaint->setChecked(true);

}

void MainWindow::OnImageSendToUpscale(QImage* SndImg)
{
    ui->tabsMain->setCurrentIndex(1);
    ui->tabsUpsOptions->setCurrentIndex(0);

    ui->lblUpscalePreImage->SetImage(SndImg);
}

void MainWindow::OnImgLabelContextMenu(QMenu* CntxMenu)
{

}

void MainWindow::OnProgressPoll()
{
    if (!CurrentPgb)
        return;

    CurrentPgb->setValue((int32_t)(CurrentAsyncSrc->state().progress * 100.f));

}

void MainWindow::OnThreadDone()
{
    IsProcessing = false;
    IterateQueue();
}

void MainWindow::OnTopBarHoverEnter(size_t LblIndex)
{
    int32_t Neighbor = GetNeighbor(LblIndex);
    if (Neighbor == -1)
        return;

    TopBarImages[Neighbor]->SetHoveringBorder(true);

}

void MainWindow::OnTopBarHoverExit(size_t LblIndex)
{
    int32_t Neighbor = GetNeighbor(LblIndex);
    if (Neighbor == -1)
        return;

    TopBarImages[Neighbor]->SetHoveringBorder(false);

}

void MainWindow::OnTopBarClick(size_t LblIndex)
{


    if (!TopBarImages.size())
        return;

    const int PREVIEW_RES = 430;

    QPixmap ViewportImg = QPixmap::fromImage(TopBarImages[LblIndex]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);


    int32_t Neighbor = GetNeighbor(LblIndex);

    if (Neighbor == -1)
    {
        ClickableImageLabel* Viewport = ui->lblLeftImg;

        if (!UseFirst)
            Viewport = ui->lblImg;

        Viewport->setPixmap(ViewportImg);
        Viewport->OriginalImage = &TopBarImages[LblIndex]->OriginalImg;
        Viewport->pToOriginalFilePath = &TopBarImages[LblIndex]->FilePath;

        UseFirst = !UseFirst;

    }
    else
    {
        QPixmap SecondViewportImg = QPixmap::fromImage(TopBarImages[Neighbor]->OriginalImg).scaled(PREVIEW_RES, PREVIEW_RES, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        ui->lblLeftImg->setPixmap(ViewportImg);
        ui->lblLeftImg->OriginalImage = &TopBarImages[LblIndex]->OriginalImg;
        ui->lblLeftImg->pToOriginalFilePath = &TopBarImages[LblIndex]->FilePath;

        ui->lblImg->setPixmap(SecondViewportImg);
        ui->lblImg->OriginalImage = &TopBarImages[Neighbor]->OriginalImg;
        ui->lblImg->pToOriginalFilePath = &TopBarImages[Neighbor]->FilePath;

        UseFirst = true;

    }
    UpdateSelectedTopBarImg(LblIndex);

    CurrentImgDisplayIndex = LblIndex;

}

std::vector<Axodox::MachineLearning::StableDiffusionSchedulerKind> ComboBoxIDToScheduler = { Axodox::MachineLearning::StableDiffusionSchedulerKind::DpmPlusPlus2M , Axodox::MachineLearning::StableDiffusionSchedulerKind::EulerAncestral };

void MainWindow::on_btnGenerate_clicked()
{
    if (!CurrentMdl.IsLoaded())
        on_btnLoadModel_clicked();

    Axodox::MachineLearning::StableDiffusionOptions Options;

    Options.BatchSize = ui->spbBatchSize->value();
    Options.GuidanceScale = (float)ui->spbCFGScale->value();

    QString Resolution = ui->edtResolution->text();
    QStringList WidthHeight = Resolution.split("x");

    Options.Height = WidthHeight[1].toInt(); Options.Width = WidthHeight[0].toInt();
    Options.PredictionType = Axodox::MachineLearning::StableDiffusionSchedulerPredictionType::V;
    Options.StepCount = ui->spbSamplingSteps->value();

    if (ui->chkImg2Img->isChecked())
    {
        float BaseDenStrength = (float)ui->sliDenoiseStrength->value();
        Options.DenoisingStrength = BaseDenStrength / 100.f;
    }

    if (ui->edtSeed->text().isEmpty())
        Options.Seed = UINT32_MAX;
    else
        Options.Seed = ui->edtSeed->text().toUInt();

    Options.Scheduler = ComboBoxIDToScheduler[ui->cbSampler->currentIndex()];

    SDOrder Ord{ ui->edtPrompt->toPlainText().toStdString(), ui->edtNegPrompt->toPlainText().toStdString(), Options, (uint32_t)ui->spbBatchCount->value(), ui->edtSeed->text().isEmpty() };
    if (ui->chkImg2Img->isChecked())
        Ord.InputImage = ui->widInpaintCanvas->getImage().copy();
    if (ui->chkInpaint->isChecked())
        Ord.InputMask = ui->widInpaintCanvas->getMask().copy();

    TaskQueue.push(Ord);

    ui->lblAllGensProgress->setText(
        QString::number(TaskQueue.size() + 1) + " orders in queue\n" +
        "Image " + QString::number(CurrentImageNumber) + "/" + QString::number(ui->pgbAllGens->maximum())
    );

    IterateQueue();
}


void MainWindow::on_btnLoadModel_clicked()
{
    QString appDirPath = QCoreApplication::applicationDirPath();
    QString fullModelPath = appDirPath + "/models/" + ui->edtModelPath->currentText();

    if (!LoadingFromModelsFolder)
        fullModelPath = ui->edtModelPath->currentText();

    qDebug() << "Model path:" << fullModelPath;

    QFile modelFile(fullModelPath);
    if (!modelFile.exists()) {
        qDebug() << "Model file does not exist at path:" << fullModelPath;
        return;
    }

    try {
        CurrentMdl.Load(fullModelPath.toStdString());
    }
    catch (const Ort::Exception& e) {
        qDebug() << "Ort::Exception caught:" << e.what();
    }
}


int32_t MainWindow::GetNeighbor(size_t InIdx)
{
    size_t MaxIndex = TopBarImages.size() - 1;

    size_t AttemptIndex = InIdx + 1;

    if (AttemptIndex > MaxIndex)
        return -1;

    return (int32_t)AttemptIndex;
}

void MainWindow::IterateQueue()
{
    if (!TaskQueue.size() && !IsProcessing)
        ui->btnCancel->hide();
    else
        ui->btnCancel->show();

    if (IsProcessing)
        return;

    if (!TaskQueue.size()) {
        CurrentItemNumber = 0;
        return;
    }

    SDOrder CuOrd = TaskQueue.front();

    Inferer* InferThrd = new Inferer;

    if (CurrentAsyncSrc->is_cancelled())
        CurrentAsyncSrc = std::make_unique<Axodox::Threading::async_operation_source>();

    InferThrd->Opts = CuOrd.Options;
    InferThrd->Model = &CurrentMdl;
    InferThrd->Prompt = CuOrd.Prompt;
    InferThrd->NegativePrompt = CuOrd.NegativePrompt;
    InferThrd->BatchCount = CuOrd.BatchCount;
    InferThrd->RandomSeed = CuOrd.RandomSeed;

    if (!CuOrd.InputImage.isNull())
        InferThrd->InputImage = CuOrd.InputImage.copy();
    if (!CuOrd.InputMask.isNull())
        InferThrd->InputMask = CuOrd.InputMask.copy();

    if (CuOrd.IsUpscale)
    {
        CurrentUpscaler.SetEnv(CurrentMdl.GetEnv());
        InferThrd->EsrGan = &CurrentUpscaler;
        CurrentPgb = ui->pgbUpscaleProg;

    }
    else {
        InferThrd->EsrGan = nullptr;
        CurrentPgb = ui->pgbCurrentGen;
    }

    connect(InferThrd, &Inferer::Done, this, &MainWindow::OnImageDone);
    connect(InferThrd, &Inferer::ThreadFinished, this, &MainWindow::OnThreadDone);

    connect(InferThrd, &Inferer::finished, InferThrd, &QObject::deleteLater);

    CurrentInferThrd = InferThrd;
    ui->pgbCurrentGen->setRange(0, 100);
    ui->pgbUpscaleProg->setRange(0, 100);

    InferThrd->AsyncSrc = CurrentAsyncSrc.get();
    InferThrd->start();

    ++CurrentItemNumber;

    ui->pgbAllGens->setRange(0, CuOrd.BatchCount);


    TaskQueue.pop();
    IsProcessing = true;

    ui->lblAllGensProgress->setText(
        QString::number(TaskQueue.size()) + " orders in queue\n" +
        "Image 0/" + QString::number(CuOrd.BatchCount)
    );

    CurrentImageNumber = 0;
}

void MainWindow::UpdateModelListing()
{
    QDir appDir(QCoreApplication::applicationDirPath());

    QDir modelsDir(appDir.absoluteFilePath("models"));

    if (!modelsDir.exists()) {
        LoadingFromModelsFolder = false;
        ui->edtModelPath->setEditable(!LoadingFromModelsFolder);
        return;
    }

    modelsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList folders = modelsDir.entryInfoList();

    ui->edtModelPath->setCurrentText("");
    ui->edtModelPath->clear();
    foreach(const QFileInfo & folder, folders) {
        ui->edtModelPath->addItem(folder.fileName());
    }

    LoadingFromModelsFolder = true;
    ui->edtModelPath->setEditable(!LoadingFromModelsFolder);
}

void MainWindow::UpdateUpscalerListing() {
    QDir appDir(QCoreApplication::applicationDirPath());

    QDir upscalersDir(appDir.absoluteFilePath("upscalers"));

    if (!upscalersDir.exists()) {
        ui->cbUpscalerModels->setDisabled(true);
        return;
    }

    upscalersDir.setNameFilters(QStringList() << "*.onnx");
    upscalersDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QFileInfoList files = upscalersDir.entryInfoList();

    ui->cbUpscalerModels->setCurrentText("");
    ui->cbUpscalerModels->clear();

    foreach(const QFileInfo & file, files) {
        QString modelName = file.baseName();
        ui->cbUpscalerModels->addItem(modelName);
    }

    ui->cbUpscalerModels->setDisabled(false);

    if (!files.isEmpty()) {
        ui->cbUpscalerModels->setCurrentIndex(0);
    }
}



void MainWindow::UpdateSelectedTopBarImg(size_t NewSelected)
{
    for (auto*& ImgWid : TopBarImages)
        ImgWid->SetSelectedBorder(false);

    TopBarImages[NewSelected]->SetSelectedBorder(true);

    int32_t Neighbor = GetNeighbor(NewSelected);

    if (Neighbor != -1)
        TopBarImages[Neighbor]->SetSelectedBorder(true);
}

void MainWindow::ResetViewports()
{
    const int PREVIEW_RES = 430;

    QPixmap WhiteFillPixm = QPixmap(PREVIEW_RES, PREVIEW_RES);
    WhiteFillPixm.fill(Qt::white);

    ui->lblLeftImg->setPixmap(WhiteFillPixm);
    ui->lblLeftImg->OriginalImage = nullptr;
    ui->lblLeftImg->pToOriginalFilePath = nullptr;

    ui->lblImg->setPixmap(WhiteFillPixm);
    ui->lblImg->OriginalImage = nullptr;
    ui->lblImg->pToOriginalFilePath = nullptr;
}

void MainWindow::OnImg2ImgEnabled()
{
    const int PREVIEW_RES = 430;

    QImage white(PREVIEW_RES, PREVIEW_RES, QImage::Format_RGBA8888);
    white.fill(Qt::white);
    ui->widInpaintCanvas->loadImage(white);
}


void MainWindow::on_btnImagesForward_clicked()
{
    size_t MaxIndex = TopBarImages.size() - 1;
    size_t DoubleHopIndex = CurrentImgDisplayIndex + 2;
    size_t SingleHopIndex = CurrentImgDisplayIndex + 1;

    if (DoubleHopIndex < MaxIndex)
        OnTopBarClick(DoubleHopIndex);
    else if (SingleHopIndex < MaxIndex)
        OnTopBarClick(SingleHopIndex);
    else
        return;
}


void MainWindow::on_btnImagesBackwards_clicked()
{
    size_t MinIndex = 0;
    int32_t DoubleHopIndex = CurrentImgDisplayIndex - 2;

    DoubleHopIndex = std::max<int32_t>(DoubleHopIndex, 0);
    OnTopBarClick(DoubleHopIndex);
}

void MainWindow::on_actionScroll_Left_triggered()
{
    on_btnImagesBackwards_clicked();
}

void MainWindow::on_actionScroll_Right_triggered()
{
    on_btnImagesForward_clicked();
}

void MainWindow::on_actionOpen_outputs_directory_triggered()
{
    QString winPath = QDir::toNativeSeparators(OutpsDir);

    QStringList args;
    args << winPath;
    QProcess::startDetached("explorer", args);

}

void MainWindow::on_actionClear_current_outputs_2_triggered()
{
    if (PreviewsSpacer) {
        ui->scraLayout->removeItem(PreviewsSpacer);
        delete PreviewsSpacer;
        PreviewsSpacer = nullptr;
    }

    for (auto* ImgPreview : TopBarImages)
    {
        ui->scraLayout->removeWidget(ImgPreview);
        delete ImgPreview;

    }
    TopBarImages.clear();

    ResetViewports();
    CurrentImgDisplayIndex = 0;
}

void MainWindow::on_btnCancel_clicked()
{
    CurrentAsyncSrc->cancel();
}

void MainWindow::on_actionRefresh_model_listing_triggered()
{
    UpdateModelListing();
}

void MainWindow::on_chkImg2Img_stateChanged(int arg1)
{
    bool Enabled = (bool)arg1;

    ui->grpImg2Img->setVisible(Enabled);

    ui->widInpaintCanvas->setVisible(Enabled);
    ui->lblImgArrow->setVisible(Enabled);

    ui->btnClearInpaint->setVisible(Enabled);


    if (Enabled) {
        ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);
        OnImg2ImgEnabled();

    }
    else {
        ui->horizontalSpacer->changeSize(40, 20, QSizePolicy::Expanding);
        ui->horizontalSpacer_2->changeSize(40, 20, QSizePolicy::Maximum);

    }
    update();
}

void MainWindow::on_sliDenoiseStrength_valueChanged(int value)
{
    ui->lblDenoisePercShow->setText(QString::number(value) + "%");
}


void MainWindow::on_chkInpaint_stateChanged(int arg1)
{
    ui->widInpaintCanvas->setPaintingEnabled((bool)arg1);
}

void MainWindow::on_btnClearInpaint_clicked()
{

    ui->widInpaintCanvas->clearStrokes();
}

void MainWindow::on_btnUpscale_clicked()
{

    if (!CurrentUpscaler.IsLoaded())
        on_btnLoadUpscaler_clicked();
    SDOrder Ord{ ui->edtPrompt->toPlainText().toStdString(), ui->edtNegPrompt->toPlainText().toStdString(), Axodox::MachineLearning::StableDiffusionOptions{}, (uint32_t)ui->spbBatchCount->value(), ui->edtSeed->text().isEmpty() };

    Ord.InputImage = *ui->lblUpscalePreImage->OriginalImage;
    Ord.IsUpscale = true;

    TaskQueue.push(Ord);
    IterateQueue();
}

void MainWindow::on_btnLoadUpscaler_clicked()
{
    if (!CurrentMdl.GetEnv())
        CurrentMdl.LoadMinimal();

    CurrentUpscaler.SetEnv(CurrentMdl.GetEnv());

    CurrentUpscaler.Load(
        QString(QCoreApplication::applicationDirPath() + "/upscalers/" + ui->cbUpscalerModels->currentText() + ".onnx").toStdString()
    );
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "inferer.h"
#include <QHBoxLayout>
#include <QTimer>
#include <queue>
#include "topbarimg.h"
#include <QWindow>
#include <QProgressBar>
QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

struct SDOrder {
    std::string Prompt;
    std::string NegativePrompt;
    Axodox::MachineLearning::StableDiffusionOptions Options;
    uint32_t BatchCount;
    bool RandomSeed;
    QImage InputImage;
    QImage InputMask;
    bool IsUpscale = false;

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void showEvent(QShowEvent* event) override;

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QWindow* ParentGoodWin;

public slots:
    void OnImageDone(QImage InImg, StableDiffusionJobType JobType);
    void OnInpaintWidImageSet();
    void OnImageSendToImg2Img(QImage* SndImg);
    void OnImageSendToInpaint(QImage* SndImg);
    void OnImageSendToUpscale(QImage* SndImg);
    void OnImgLabelContextMenu(QMenu* CntxMenu);


private slots:
    void OnProgressPoll();
    void OnThreadDone();

    void OnTopBarHoverEnter(size_t LblIndex);
    void OnTopBarHoverExit(size_t LblIndex);
    void OnTopBarClick(size_t LblIndex);

private slots:
    void on_btnGenerate_clicked();

    void on_btnLoadModel_clicked();


    void on_btnImagesForward_clicked();

    void on_btnImagesBackwards_clicked();

    void on_actionScroll_Left_triggered();

    void on_actionScroll_Right_triggered();

    void on_actionOpen_outputs_directory_triggered();

    void on_actionClear_current_outputs_2_triggered();

    void on_btnCancel_clicked();

    void on_actionRefresh_model_listing_triggered();

    void on_chkImg2Img_stateChanged(int arg1);

    void on_sliDenoiseStrength_valueChanged(int value);

    void on_chkInpaint_stateChanged(int arg1);

    void on_btnClearInpaint_clicked();

    void on_btnUpscale_clicked();

    void on_btnLoadUpscaler_clicked();

private:
    bool DidFirstShowStuff;
    int32_t GetNeighbor(size_t InIdx);
    void IterateQueue();
    std::queue<SDOrder> TaskQueue;
    QTimer* ProcessTimer;
    bool IsProcessing;
    uint32_t CurrentItemNumber;
    uint32_t CurrentImageNumber;

    bool LoadingFromModelsFolder;
    size_t CurrentImgDisplayIndex;
    std::vector<TopBarImg*> TopBarImages;

    QProgressBar* CurrentPgb;
    void UpdateModelListing();
    void UpdateSelectedTopBarImg(size_t NewSelected);
    void ResetViewports();
    void OnImg2ImgEnabled();

private:
    QTimer* progressPoller;
    std::unique_ptr<Axodox::Threading::async_operation_source> CurrentAsyncSrc;
    QString OutpsDir;
    QSpacerItem* PreviewsSpacer;
    bool UseFirst;
    StableDiffusionModel CurrentMdl;
    Upscaler CurrentUpscaler;
    Ui::MainWindow* ui;
    Inferer* CurrentInferThrd;
    void UpdateUpscalerListing();
};
#endif // MAINWINDOW_H
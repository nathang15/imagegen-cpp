#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Inferer.h"
#include <QHBoxLayout>
#include <QTimer>
#include <queue>
#include "TopbarImg.h"
#include <QProgressBar>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

using namespace std;

struct sdOrder {
    string prompt;
    string negPrompt;
    Axodox::MachineLearning::StableDiffusionOptions options;
    uint32_t batchCount;
    bool randSeed;
    QImage inImage;
    QImage inMask;
    bool isUpscale = false;

};

class MainWindow : public QMainWindow {
    Q_OBJECT

protected:
    void showEvent(QShowEvent* event) override;

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    QWindow* ParentWin;

public slots:
    void onImgDone(QImage inImg, SDJobType jobType);
    void onInpaintWidImgSet();
    void onSendImg2Img(QImage* img);
    void onSendToInpaint(QImage* img);
    void onSendToUpscale(QImage* img);
    void onLabelCtxMenu(QMenu* ctxMenu);


private slots:
    void onProgressPoll();
    void onThreadDone();

    void onTopBarHoverEnter(size_t labelIndex);
    void onTopBarHoverExit(size_t labelIndex);
    void onTopBarClick(size_t labelIndex);

private slots:
    void onGenerateBtnClicked();
    void onLoadModelBtnClicked();
    void onImgFwdBtnClicked();
    void onImgBwdBtnClicked();
    void onScrollLeft();
    void onScrollRight();
    void onOpenOutputsFolder();
    void onClearCurOutput2();
    void onCancelBtnClicked();
    void onRefreshModelsList();
    void onCheckImg2ImgState(int arg);
    void onDenoiseStrengthValueSet(int val);
    void onCheckInpaintState(int arg);
    void onClearInpaintBtnClicked();
    void onUpscaleBtnClicked();
    void onLoadUpscalerBtnClicked();

private:
    bool FirstShowed;
    int32_t getNeighbor(size_t inIndex);
    void iterateQueue();
    std::queue<sdOrder> taskQueue;
    QTimer* processTimer;
    bool processing;
    uint32_t curItemNum;
    uint32_t curImgNum;
    bool loadingModels;
    size_t curImgDisplayIndex;
    std::vector<TopbarImg*> topBarImgs;
    QProgressBar* curPgb;
    void updateModelsList();
    void updateSelectedTopBarImg(size_t NewSelected);
    void resetViewports();
    void onImg2ImgEnabled();

private:
    QTimer* progressPoller;
    std::unique_ptr<Axodox::Threading::async_operation_source> curAsyncSrc;
    QString outDir;
    QSpacerItem* previews;
    bool useFirst;
    StableDiffusionModel curModel;
    Upscaler curUpscaler;
    Ui::MainWindow* ui;
    Inferer* curInferThread;
    void updateUpscalerList();
};
#endif // MAINWINDOW_H
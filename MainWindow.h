#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QTimer>
#include <queue>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

using namespace std;

struct sdOrder {
    string prompt;
    string negPrompt;
    //Axodox::MachineLearning::StableDiffusionOptions options;
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
    void onDone(QImage inImg, sdJobType jobType);
    void onInpaintWidImgSet();
    void onSendImg2Img(QImage* img);
    void onSendToInpaint(QImage* img);
    void onSendToUpscale(QImage* img);
    void onLabelCtxMenu(QMenu* ctxMenu);


private slots:
    void onProgressPoll();
    void onThreadDone();

    void onTopBarHoverEnter(size_t lblIndex);
    void onTopBarHoverExit(size_t lblIndex);
    void onTopBarClick(size_t lblIndex);

private slots:
    void onGenerateBtnClicked();
    void onLoadModelBtnClicked();
    void onImgFwdBtnClicked();
    void ImgBwdBtnClicked();
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
    bool loadingModels();
    size_t curImgDisplayIndex;
    std::vector<TopImg*> topImgs;

private:
    QTimer* progressPoller;
    std::unique_ptr<Axodox::Threading::async_operation_source> curAsyncSrc;
    QString outDir;
    QSpacerItem* previews;
    bool useFirst;
    sdModel curModel;
    Upscaler curUpscaler;
    Inferer* curInferThread;
    void updateUpscalerList();
};
#endif // MAINWINDOW_H
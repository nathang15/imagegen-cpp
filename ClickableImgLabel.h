#pragma once
#ifndef CLICKABLEIMGLABEL_H
#define CLICKABLEIMGLABEL_H

#include <QLabel>
#include <QImage>
#include <QMouseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>

class ClickableImgLabel : public QLabel {
    Q_OBJECT

public:
    QImage* originalImg;
    QString* pointerToFilePath;


    explicit ClickableImgLabel(QWidget* parent = nullptr);
    ~ClickableImgLabel() override;

    void loadImage(const QString& imgPath);
    void setImage(QImage* img);

private:
    bool ownsImg;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;


signals:
    void onMenuOpen(QMenu* inMenu);
    void sendToInpaint(QImage* img);
    void sendToImg2Img(QImage* img);
    void sendToUpscale(QImage* img);


private slots:
    void showInFolder();

    void onClickSendToImg2Img();
    void onClickSendToInpaint();
    void onClickSendToUpscale();

private:
    QAction* sendToInpaint;
    QAction* sendToImg2Img;
    QAction* sendToUpscale;

};

#endif
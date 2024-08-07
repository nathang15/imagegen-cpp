#ifndef CLICKABLEIMAGELABEL_H
#define CLICKABLEIMAGELABEL_H

#include <QLabel>
#include <QImage>
#include <QMouseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>

class ClickableImageLabel : public QLabel {
    Q_OBJECT

public:
    QImage* OriginalImage;
    QString* pToOriginalFilePath;


    explicit ClickableImageLabel(QWidget* parent = nullptr);
    ~ClickableImageLabel() override;

    void loadImage(const QString& imagePath);
    void SetImage(QImage* Img);

private:


    bool OwnsImage;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;


signals:
    void OnMenuOpen(QMenu* InMenu);

    void SendImageToInpaint(QImage* Img);
    void SendImageToImg2Img(QImage* Img);
    void SendImageToUpscale(QImage* Img);


private slots:
    void showInFolder();

    void OnClickSendToImg2Img();
    void OnClickSendToInpaint();
    void OnClickSendToUpscale();

private:
    QAction* actSendToInpaint;
    QAction* actSendToImg2Img;
    QAction* actSendToUpscale;

};

#endif // CLICKABLEIMAGELABEL_H
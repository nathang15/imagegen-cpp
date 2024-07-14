#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>

class Canvas : public QWidget {
    Q_OBJECT

public:
    explicit Canvas(QWidget* parent = nullptr);
    void loadImg(const QString& imgPath);
    void loadImg(const QImage& img);
    void setPaintingEnabled(bool enabled);
    void clearStrokes();

    QImage getImg() const;
    QImage getMask() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

private:
    void init();
    void setCustomCursor();
    int brushSize;
    QImage canvasImage;
    QImage originalImage;
    QImage maskImage;
    QImage displayedImage;

    bool paintingEnabled = false;
    QPoint lastPoint;

signals:
    void OnImageSet();
};

#endif
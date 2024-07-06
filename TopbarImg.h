#ifndef TOPBARIMG_H
#define TOPBARIMG_H

#include <QLabel>
#include <QObject>
#include <QImage>
class TopbarImg : public QLabel
{
    Q_OBJECT

public:
    explicit TopbarImg(
        QWidget* parent = Q_NULLPTR, 
        Qt::WindowFlags f = Qt::WindowFlags()
    );
    ~TopbarImg() override;
    TopbarImg();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

public:
    size_t VecIndex;
    QImage OriginalImg;
    QString FilePath;

    void setHoveringBorder(bool hovering);
    void setSelectedBorder(bool selected);

signals:
    void hoverEnter(size_t labelIndex);
    void hoverExit(size_t labelIndex);
    void mouseClicked(size_t labelIndex);

private:
    bool curSelected;
};

#endif
#ifndef HORIZONTALSCROLLAREA_H
#define HORIZONTALSCROLLAREA_H

#include <QScrollArea>

class QWidget;

class HorizontalScrollArea : public QScrollArea {
    Q_OBJECT

public:
    explicit horizontalScrollArea(QWidget* parent = nullptr);

    void registerContentsWidget(QWidget* scraWidContents);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QWidget* m_scrollAreaWidgetContents;
};

#endif
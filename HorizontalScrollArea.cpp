#include "HorizontalScrollArea.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QEvent>

HorizontalScrollArea::horizontalScrollArea(QWidget* parent) : QScrollArea(parent)
{
	setWidgetResizable(true);
	setFrameStyle(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scrollAreaWidgetContents = nullptr;
}

void HorizontalScrollArea::registerContentsWidget(QWidget* scraWidContents)
{
	m_scrollAreaWidgetContents = scraWidContents;
	m_scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	m_scrollAreaWidgetContents->installEventFilter(this);
}

bool HorizontalScrollArea::eventFilter(QObject* obj, QEvent* event)
{
	if (obj == m_scrollAreaWidgetContents && event->type() == QEvent::Resize)
	{
		int32_t scrollBarHeight = verticalScrollBar()->height();

		if (!horizontalScrollBar()->isVisible())
		{
			scrollBarHeight = 0;
		}
		setMinimumHeight(m_scrollAreaWidgetContents->minimumSizeHint().height() + scrollBarHeight);
	}
	return QScrollArea::eventFilter(obj, event);
}

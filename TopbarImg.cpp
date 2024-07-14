#include "TopbarImg.h"

TopbarImg::TopbarImg(QWidget* parent, Qt::WindowFlags f) :QLabel(parent, f)
{
	setAttribute(Qt::WA_Hover);
	curSelected = false;
}

TopbarImg::~TopbarImg() {}

TopbarImg::TopbarImg() {}

void TopbarImg::enterEvent(QEnterEvent* event)
{
	QLabel::enterEvent(event);

	if (curSelected)
		return;

	setHoveringBorder(true);
	emit hoverEnter(VecIndex);
}

void TopbarImg::leaveEvent(QEvent* event)
{
	QLabel::leaveEvent(event);

	if (curSelected)
		return;

	setHoveringBorder(false);
	emit hoverExit(VecIndex);
}

void TopbarImg::mousePressEvent(QMouseEvent* event)
{
	QLabel::mousePressEvent(event);
	emit mouseClicked(VecIndex);

}

void TopbarImg::setHoveringBorder(bool hovering)
{
	if (hovering)
		setStyleSheet("border: 2px solid yellow;");
	else
		setStyleSheet("");
}

void TopbarImg::setSelectedBorder(bool selected)
{
	if (selected)
		setStyleSheet("border: 3px solid blue;");
	else
		setStyleSheet("");

	curSelected = selected;
}
#pragma once

#include <QtGui/QGraphicsTextItem>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtGui/QPainter>
#include <QtGui/QFont>

#include "item.h"

class Text : public Item
{
public:
	Text(bool isDynamic = false);
	Text(int x, int y, QString const &text = "text", bool isDynamic = false);
	void init(int x, int y, QString const &text);
	bool isDynamicText();
	virtual void setIsDynamicText(bool isDynamic);
	virtual QRectF boundingRect() const;
	virtual QRectF realBoundingRect() const;
	virtual void drawItem(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void drawExtractionForItem(QPainter* painter);
	virtual void drawFieldForResizeItem(QPainter* painter);
	virtual void drawScalingRects(QPainter* painter);
	virtual void changeScalingPointState(qreal x, qreal y);
	QGraphicsTextItem const& getText();
	void setZValueAll(int const &index);

	virtual QPair<QDomElement, Item::DomElementTypes> generateItem(QDomDocument &document, QPoint const &topLeftPicture);

protected:
	int mX1;
	int mY1;
	QGraphicsTextItem mText;
	bool mIsDynamicText;
	QRectF mRect;
	void drawForDynamicText(QPainter* painter);
};

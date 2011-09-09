#include "tabbarlayout.h"

#include <QWidget>
#include "tabbaritem.h"
#include <utils/stylestorage.h>

TabBarLayout::TabBarLayout(QWidget *AParent) : QLayout(AParent)
{
	FMinWidth = 80;
	FMaxWidth = 180;
	FItemsWidth = FMaxWidth;
	FStreatch = false;
	FUpdateBlocked = false;

	setMargin(0);
	setSpacing(0);
}

TabBarLayout::~TabBarLayout()
{
	while (count()>0)
		delete takeAt(0);
}

int TabBarLayout::minimumItemWidth() const
{
	return FMinWidth;
}

int TabBarLayout::maximumItemWidth() const
{
	return FMaxWidth;
}

void TabBarLayout::setMinMaxItemWidth(int AMin, int AMax)
{
	if (AMin>0 && AMax>AMin)
	{
		FMinWidth = AMin;
		FMaxWidth = AMax;
		updateLayout();
	}
}

void TabBarLayout::blockUpdate(bool ABlock)
{
	FUpdateBlocked = ABlock;
	updateLayout();
}

void TabBarLayout::updateLayout()
{
	if (FUpdateBlocked)
	{
		int fakeItemWidth;
		calcLayoutParams(geometry().width(),fakeItemWidth,FStreatch);
	}
	else
	{
		calcLayoutParams(geometry().width(),FItemsWidth,FStreatch);
	}
	doLayout(geometry(),FItemsWidth,FStreatch,true);
}

int TabBarLayout::indexToOrder(int AIndex) const
{
	return FItemsOrder.indexOf(FItems.value(AIndex));
}

int TabBarLayout::orderToIndex(int AOrder) const
{
	return FItems.indexOf(FItemsOrder.value(AOrder));
}

void TabBarLayout::moveItem(int ATarget, int ADestination)
{
	if (ATarget!=ADestination && ATarget>=0 && ATarget<FItems.count() && ADestination>=0 && ADestination<FItems.count())
	{
		FItemsOrder.move(FItemsOrder.indexOf(FItems.at(ATarget)), FItemsOrder.indexOf(FItems.at(ADestination)));
		updateLayout();
	}
}

int TabBarLayout::count() const
{
	return FItems.count();
}

void TabBarLayout::addItem(QLayoutItem *AItem)
{
	FItems.append(AItem);
	FItemsOrder.append(AItem);
	updateLayout();
}

QLayoutItem *TabBarLayout::itemAt(int AIndex) const
{
	return FItems.value(AIndex, NULL);
}

QLayoutItem *TabBarLayout::takeAt(int AIndex)
{
	QLayoutItem *item = NULL;
	if (AIndex>=0 && AIndex<FItems.count())
	{
		item = FItems.takeAt(AIndex);
		FItemsOrder.removeAll(item);
	}
	return item;
}

QSize TabBarLayout::sizeHint() const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);

	int height = 0;
	foreach(QLayoutItem *item, FItemsOrder)
		height = qMax(height, item->sizeHint().height());
	int width = (FMaxWidth + spacing()) * FItems.count() - spacing();

	return QSize(left + width + right, top + height + bottom);
}

bool TabBarLayout::hasHeightForWidth() const
{
	return true;
}

int TabBarLayout::heightForWidth(int AWidth) const
{
	int itemWidth;
	bool streatch;
	calcLayoutParams(AWidth,itemWidth,streatch);
	return doLayout(QRect(0,0,AWidth,0),itemWidth,streatch,false);
}

Qt::Orientations TabBarLayout::expandingDirections() const
{
	return Qt::Horizontal;
}

void TabBarLayout::setGeometry(const QRect &ARect)
{
	QLayout::setGeometry(ARect);
	updateLayout();
}

#define LINES(itms,ipl) ((ipl)>0 ? (itms)/(ipl) + ((itms)%(ipl)>0 ? 1 : 0) : 1)
void TabBarLayout::calcLayoutParams(int AWidth, int &AItemWidth, bool &AStretch) const
{
	int left, right;
	getContentsMargins(&left, NULL, &right, NULL);

	int availWidth = AWidth - left - right - 1;

	if (!FItems.isEmpty() && (FMaxWidth + spacing()) * FItems.count() - spacing() >= availWidth)
	{
		int itemsPerLine = qMin(availWidth / (FMinWidth + spacing()), FItems.count());
		int lines = LINES(FItems.count(), itemsPerLine);
		while (itemsPerLine>1 && lines==LINES(FItems.count(),itemsPerLine-1))
			itemsPerLine--;
		AItemWidth = itemsPerLine>0 ? (availWidth-((itemsPerLine-1)*spacing())) / itemsPerLine : FMinWidth;
		AStretch = true;
	}
	else
	{
		AItemWidth = FMaxWidth;
		AStretch = false;
	}
}

int TabBarLayout::doLayout(QRect ARect, int AItemWidth, bool AStretch, bool AResize) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);

	QRect availRect = ARect.adjusted(+left,+top,-right,-bottom);

	int x = availRect.left();
	int y = availRect.top();
	int lineHeight = 0;

	foreach(QLayoutItem *item, FItemsOrder)
	{
		QRect itemRect = QRect(x,y,AItemWidth,item->sizeHint().height());
		lineHeight = qMax(lineHeight, itemRect.height());
		x += AItemWidth + spacing();

		if ( x + AItemWidth - spacing() > availRect.right() )
		{
			if (item != FItemsOrder.last())
			{
				y += lineHeight + spacing();
				lineHeight = 0;
			}
			if (AStretch)
			{
				itemRect.setRight(availRect.right());
			}
			x = availRect.left();
		}

		if (AResize)
			item->setGeometry(itemRect);
	}

	if (AResize)
	{
		foreach(QLayoutItem *item, FItemsOrder)
		{
			TabBarItem *tabBarItem = qobject_cast<TabBarItem*>(item->widget());
			if (tabBarItem)
			{
				QRect itemRect = item->geometry();
				tabBarItem->setLeft(itemRect.left() == availRect.left());
				tabBarItem->setRight((itemRect.right() == availRect.right()) || (item == FItemsOrder.last()));
				tabBarItem->setTop(itemRect.top() == availRect.top());
				tabBarItem->setBottom(itemRect.bottom() == availRect.bottom());
			}
		}
	}

	return y - availRect.top() + lineHeight + top + bottom;
}

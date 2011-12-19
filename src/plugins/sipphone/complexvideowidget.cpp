#include "complexvideowidget.h"
#include <QPainter>
#include <QPaintEvent>

ComplexVideoWidget::ComplexVideoWidget(QWidget *parent) : QWidget(parent), _noSignal(false), _pPixmap(NULL)
{

	setObjectName("ComplexVideoWidget");

}

ComplexVideoWidget::~ComplexVideoWidget()
{
	if(_pPixmap)
	{
		delete _pPixmap;
		_pPixmap = NULL;
	}
}

QPixmap* ComplexVideoWidget::picture() const
{
	return _pPixmap;
}

void ComplexVideoWidget::setPicture(const QImage& img)
{
	setPicture(QPixmap::fromImage(img));
}


void ComplexVideoWidget::setPicture(const QPixmap& pxmp)
{
	//if (!d->pixmap || d->pixmap->cacheKey() != pixmap.cacheKey()) {
	//	d->clearContents();
	//	d->pixmap = new QPixmap(pixmap);
	//}

	//if (d->pixmap->depth() == 1 && !d->pixmap->mask())
	//	d->pixmap->setMask(*((QBitmap *)d->pixmap));

	//d->updateLabel();

	if(_noSignal)
		_noSignal = false;

	//static int k=0;
	//k++;
	//if(k > 50)
	//	return;

	if(!_pPixmap || _pPixmap->cacheKey() != pxmp.cacheKey())
	{
		if(_pPixmap != NULL)
		{
			delete _pPixmap;
			_pPixmap = NULL;
		}
		_pPixmap = new QPixmap(pxmp);
		update(rect());
	}
}

void ComplexVideoWidget::paintEvent(QPaintEvent *ev)
{
	Q_UNUSED(ev);
	//QWidget::paintEvent(ev);

	QPainter painter(this);

	painter.fillRect(/*ev->*/rect(), Qt::black);
	// ��� ����������� ��� �����������
	if(_noSignal || _pPixmap == NULL)
	{
		QRect currRect = /*ev->*/rect();
		QSize currSize = size();
		int x = currRect.x() + currRect.width()/2;
		int y = currRect.y() + currRect.height()/2;
		Q_UNUSED(currSize);
		Q_UNUSED(x);
		Q_UNUSED(y);
		//QRect pixRect(QPoint(x,y), QSize(scaledPixmap.width(), scaledPixmap.height()));
		painter.setPen(Qt::white);
		painter.drawText(currRect, Qt::AlignCenter, tr("Camera OFF"));
	}
	else
	{
		QRect currRect = /*ev->*/rect();
		QSize currSize = size();
		QPixmap scaledPixmap = _pPixmap->scaled(currSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		int x = currRect.x() + currRect.width()/2 - scaledPixmap.width()/2;
		int y = currRect.y() + currRect.height()/2 - scaledPixmap.height()/2;
		QRect pixRect(QPoint(x,y), QSize(scaledPixmap.width(), scaledPixmap.height()));
		painter.drawPixmap(pixRect, scaledPixmap);
	}

}

//void ComplexVideoWidget::moveEvent(QMoveEvent *ev)
//{
//	QWidget::moveEvent(ev);
//update();
//updateGeometry();
//}
//
//void ComplexVideoWidget::resizeEvent(QResizeEvent *ev)
//{
//	QWidget::resizeEvent(ev);
//	update();
//	updateGeometry();
//}

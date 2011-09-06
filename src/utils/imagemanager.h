#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include "utilsexport.h"
#include <QImage>

class UTILS_EXPORT ImageManager
{
public:
	static QImage grayscaled(const QImage & image);
	static QImage squared(const QImage & image, int size);
	static QImage roundSquared(const QImage & image, int size, int radius);
	static QImage addShadow(const QImage & image, QColor color, QPoint offset, bool canResize = false);
	static QImage colorized(const QImage & image, QColor color);
	static QImage opacitized(const QImage & image, double opacity = 0.5);
	static void drawNinePartImage(const QImage &image, QRectF paintRect, qreal borderLeft, qreal borderRight, qreal borderTop, qreal borderBottom, QPainter * painter);
	static void drawNinePartImage(const QImage &image, QRectF paintRect, qreal border, QPainter * painter);
};

#endif // IMAGEMANAGER_H

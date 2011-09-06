#ifndef MACDOCKHANDLER_H
#define MACDOCKHANDLER_H

#include <QObject>
#include "utilsexport.h"

class UTILS_EXPORT MacDockHandler : public QObject
{
	Q_OBJECT
private:
	MacDockHandler();
public:
	static MacDockHandler * instance();
signals:
	void dockIconClicked();
public:
	void emitClick();

private:
	static MacDockHandler * _instance;
};

#endif // MACDOCKHANDLER_H

#ifndef CUSTOMBORDERSTORAGE_H
#define CUSTOMBORDERSTORAGE_H

#include "filestorage.h"
#include "custombordercontainer.h"

class CustomBorderContainerPrivate;

class UTILS_EXPORT CustomBorderStorage : public FileStorage
{
	Q_OBJECT
public:
	CustomBorderStorage(const QString &AStorage, const QString &ASubStorage = QString::null, QObject *AParent = NULL);
	~CustomBorderStorage();
	CustomBorderContainer * addBorder(QWidget * widget, const QString & key);
	void removeBorder(QWidget * widget);
public:
	static CustomBorderStorage * staticStorage(const QString & storage);
private:
	static QHash<QString, CustomBorderContainerPrivate *> borderStyleCache;
	static QHash<QWidget *, CustomBorderContainer *> borderCache;
	static QHash<QString, CustomBorderStorage *> staticStorages;
};

#endif // CUSTOMBORDERSTORAGE_H

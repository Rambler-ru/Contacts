#ifndef SELECTICONMENU_H
#define SELECTICONMENU_H

#include <QVBoxLayout>
#include <definitions/resources.h>
#include <utils/menu.h>
#include <utils/iconstorage.h>
#include "selecticonwidget.h"

class SelectIconMenu :
			public Menu
{
	Q_OBJECT
public:
	SelectIconMenu(const QString &AIconset, QWidget *AParent = NULL);
	~SelectIconMenu();
	QWidget *instance() { return this; }
	QString iconset() const;
	void setIconset(const QString &ASubStorage);
	IconStorage *iconStorage() const;
signals:
	void iconSelected(const QString &ASubStorage, const QString &AIconKey);
public:
	virtual QSize sizeHint() const;
protected slots:
	void onAboutToShow();
private:
	QVBoxLayout *FLayout;
	IconStorage *FStorage;
private:
	QSize FSizeHint;
};

#endif // SELECTICONMENU_H

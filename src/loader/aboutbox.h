#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QDialog>
#include <definitions/version.h>
#include <interfaces/ipluginmanager.h>
#include "ui_aboutbox.h"

class CustomBorderContainer;

class AboutBox :
	public QDialog
{
	Q_OBJECT
public:
	AboutBox(IPluginManager *APluginManager, QWidget *AParent = NULL);
	~AboutBox();
protected slots:
	void onLabelLinkActivated(const QString &ALink);
private:
	Ui::AboutBoxClass ui;
	CustomBorderContainer * border;
};

#endif // ABOUTBOX_H

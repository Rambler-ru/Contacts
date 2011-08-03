#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QObjectCleanupHandler>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/stylesheets.h>
#include <definitions/optionvalues.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/stylestorage.h>
#include <utils/iconstorage.h>
#include "ui_optionsdialog.h"

class SortFilterProxyModel :
	public QSortFilterProxyModel
{
	Q_OBJECT
public:
	SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent) {}
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class OptionsDialog :
	public QDialog
{
	Q_OBJECT
public:
	OptionsDialog(IOptionsManager *AOptionsManager, QWidget *AParent = NULL);
	~OptionsDialog();
public:
	void showNode(const QString &ANodeId);
signals:
	void applied();
	void reseted();
	void dialogDestroyed();
protected:
	QWidget *createNodeWidget(const QString &ANodeId);
	QStandardItem *createNodeItem(const QString &ANodeId);
	bool canExpandVertically(const QWidget *AWidget) const;
	void correctAdjustSize();
protected slots:
	void onOptionsDialogNodeInserted(const IOptionsDialogNode &ANode);
	void onOptionsDialogNodeRemoved(const IOptionsDialogNode &ANode);
	void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious);
	void onOptionsWidgetModified();
	void onOptionsWidgetUpdated();
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	Ui::OptionsDialogClass ui;
private:
	IOptionsManager *FManager;
private:
	QStandardItemModel *FItemsModel;
	SortFilterProxyModel *FProxyModel;
private:
	QWidget *FCurrentWidget;
	QObjectCleanupHandler FCleanupHandler;
	QMap<QString, QStandardItem *> FNodeItems;
	QMap<QStandardItem *, QWidget *> FItemWidgets;
};

#endif // OPTIONSDIALOG_H

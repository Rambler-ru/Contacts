#ifndef ITRAYMANAGER_H
#define ITRAYMANAGER_H

#include <QSystemTrayIcon>
#include <utils/menu.h>

#define TRAYMANAGER_UUID "{47d2e04f-fb5c-44e3-93ea-5d296b211293}"

struct ITrayNotify 
{
	bool blink;
	QIcon icon;
	QString iconKey;
	QString iconStorage;
	QString toolTip;
};

class ITrayManager 
{
public:
	virtual QObject *instance() =0;
	virtual QRect geometry() const =0;
	virtual Menu *contextMenu() const =0;
	virtual QIcon icon() const =0;
	virtual void setIcon(const QIcon &AIcon) =0;
	virtual QString toolTip() const =0;
	virtual void setToolTip(const QString &AToolTip) =0;
	virtual int activeNotify() const =0;
	virtual QList<int> notifies() const =0;
	virtual ITrayNotify notifyById(int ANotifyId) const =0;
	virtual int appendNotify(const ITrayNotify &ANotify) =0;
	virtual void removeNotify(int ANotifyId) =0;
	virtual void showMessage(const QString &ATitle, const QString &AMessage, QSystemTrayIcon::MessageIcon AIcon = QSystemTrayIcon::Information, int ATimeout = 10000) =0;
protected:
	virtual void notifyAppended(int ANotifyId) =0;
	virtual void notifyRemoved(int ANotifyId) =0;
	virtual void activeNotifyChanged(int ANotifyId) =0;
	virtual void notifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason) =0;
	virtual void messageClicked() =0;
	virtual void messageShown(const QString &ATitle, const QString &AMessage,QSystemTrayIcon::MessageIcon AIcon, int ATimeout) =0;
};

Q_DECLARE_INTERFACE(ITrayManager,"Virtus.Plugin.ITrayManager/1.0")

#endif

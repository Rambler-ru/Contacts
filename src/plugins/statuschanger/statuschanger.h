#ifndef STATUSCHANGER_H
#define STATUSCHANGER_H

#include <QSet>
#include <QPair>
#include <QPointer>
#include <QDateTime>
#include <definitions/plugininitorders.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/stylesheets.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/itraymanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/inotifications.h>
#include <interfaces/ivcard.h>
#include <interfaces/iavatars.h>
#include <utils/options.h>
#include <utils/log.h>
#include "statuswidget.h"
#include "customstatusdialog.h"

struct StatusItem
{
	StatusItem()
	{
		code = STATUS_NULL_ID;
		show = IPresence::Offline;
		priority = 0;
	}
	int code;
	QString name;
	int show;
	QString text;
	int priority;
	QDateTime lastActive;
};

class StatusChanger :
	public QObject,
	public IPlugin,
	public IStatusChanger
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IStatusChanger)
public:
	StatusChanger();
	~StatusChanger();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return STATUSCHANGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IStatusChanger
	virtual Menu *statusMenu() const;
	virtual int mainStatus() const;
	virtual void setMainStatus(int AStatusId);
	virtual int streamStatus(const Jid &AStreamJid) const;
	virtual void setStreamStatus(const Jid &AStreamJid, int AStatusId);
	virtual QString statusItemName(int AStatusId) const;
	virtual int statusItemShow(int AStatusId) const;
	virtual QString statusItemText(int AStatusId) const;
	virtual int statusItemPriority(int AStatusId) const;
	virtual QList<int> statusItems() const;
	virtual QList<int> activeStatusItems() const;
	virtual QList<int> statusByShow(int AShow) const;
	virtual int statusByName(const QString &AName) const;
	virtual int addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority);
	virtual void updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority);
	virtual void removeStatusItem(int AStatusId);
	virtual QIcon iconByShow(int AShow) const;
	virtual QString nameByShow(int AShow) const;
signals:
	void statusAboutToBeChanged(const Jid &AStreamJid, int AStatusId);
	void statusChanged(const Jid &AStreamJid, int AStatusId);
	void statusItemAdded(int AStatusId);
	void statusItemChanged(int AStatusId);
	void statusItemRemoved(int AStatusId);
protected:
	void createDefaultStatus();
	void setMainStatusId(int AStatusId);
	void setStreamStatusId(IPresence *APresence, int AStatusId);
	Action *createStatusAction(int AStatusId, const Jid &AStreamJid, QObject *AParent) const;
	void updateStatusAction(int AStatusId, Action *AAction) const;
	void createStatusActions(int AStatusId);
	void updateStatusActions(int AStatusId);
	void removeStatusActions(int AStatusId);
	int visibleMainStatusId() const;
	void updateMainMenu();
	int createTempStatus(IPresence *APresence, int AShow, const QString &AText, int APriority);
	void removeTempStatus(IPresence *APresence);
	void autoReconnect(IPresence *APresence);
	void resendUpdatedStatus(int AStatusId);
	void removeAllCustomStatuses();
	void removeRedundantCustomStatuses();
	void updateStatusNotification(IPresence *APresence);
	void removeStatusNotification(IPresence *APresence);
protected slots:
	void onSetStatusByAction(bool);
	void onPresenceAdded(IPresence *APresence);
	void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
	void onPresenceRemoved(IPresence *APresence);
	void onRosterOpened(IRoster *ARoster);
	void onRosterClosed(IRoster *ARoster);
	void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
	void onDefaultStatusIconsChanged();
	void onOptionsOpened();
	void onOptionsClosed();
	void onProfileOpened(const QString &AProfile);
	void onShutdownStarted();
	void onReconnectTimer();
	void onCustomStatusAction(bool);
	void onClearCustomStatusAction(bool);
	void onTrayContextMenuAboutToShow();
	void onTrayContextMenuAboutToHide();
	void onNotificationActivated(int ANotifyId);
private:
	IPluginManager *FPluginManager;
	IPresencePlugin *FPresencePlugin;
	IRosterPlugin *FRosterPlugin;
	IMainWindowPlugin *FMainWindowPlugin;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IOptionsManager *FOptionsManager;
	ITrayManager *FTrayManager;
	IAccountManager *FAccountManager;
	IStatusIcons *FStatusIcons;
	INotifications *FNotifications;
	IVCardPlugin *FVCardPlugin;
	IAvatars *FAvatars;
private:
	Menu *FStatusMenu;
	StatusWidget *FStatusWidget;
	IPresence *FChangingPresence;
	QList<IPresence *> FShutdownList;
	QMap<int, StatusItem> FStatusItems;
	QSet<IPresence *> FMainStatusStreams;
	QMap<IPresence *, int> FLastOnlineStatus;
	QMap<IPresence *, int> FCurrentStatus;
	QMap<IPresence *, int> FConnectStatus;
	QMap<IPresence *, int> FTempStatus;
	QMap<IPresence *, int> FConnectNotifyId;
	QMap<IPresence *, QPair<QDateTime,int> > FPendingReconnect;
	QPointer<CustomStatusDialog> FCustomStatusDialog;
};

#endif // STATUSCHANGER_H

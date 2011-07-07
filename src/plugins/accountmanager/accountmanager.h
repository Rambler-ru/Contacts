#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QPointer>
#include <definitions/actiongroups.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <utils/action.h>
#include "account.h"
#include "accountoptions.h"
#include "accountsoptions.h"

class AccountsOptions;

class AccountManager :
			public QObject,
			public IPlugin,
			public IAccountManager,
			public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAccountManager IOptionsHolder);
public:
	AccountManager();
	~AccountManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ACCOUNTMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IAccountManager
	virtual QList<IAccount *> accounts() const;
	virtual IAccount *accountById(const QUuid &AAcoountId) const;
	virtual IAccount *accountByStream(const Jid &AStreamJid) const;
	virtual IAccount *appendAccount(const QUuid &AAccountId);
	virtual void showAccount(const QUuid &AAccountId);
	virtual void hideAccount(const QUuid &AAccountId);
	virtual void removeAccount(const QUuid &AAccountId);
	virtual void destroyAccount(const QUuid &AAccountId);
signals:
	void appended(IAccount *AAccount);
	void shown(IAccount *AAccount);
	void hidden(IAccount *AAccount);
	void removed(IAccount *AAccount);
	void changed(IAccount *AAcount, const OptionsNode &ANode);
	void destroyed(const QUuid &AAccountId);
public:
	void showAccountOptionsDialog(const QUuid &AAccountId);
	void openAccountOptionsNode(const QUuid &AAccountId, const QString &AName);
	void closeAccountOptionsNode(const QUuid &AAccountId);
protected slots:
	void onProfileOpened(const QString &AProfile);
	void onProfileClosed(const QString &AProfile);
	void onOptionsOpened();
	void onOptionsClosed();
	void onShowAccountOptions(bool);
	void onAccountActiveChanged(bool AActive);
	void onAccountOptionsChanged(const OptionsNode &ANode);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
private:
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QMap<QUuid, IAccount *> FAccounts;
};

#endif // ACCOUNTMANAGER_H

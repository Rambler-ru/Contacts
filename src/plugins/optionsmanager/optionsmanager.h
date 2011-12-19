#ifndef OPTIONSMANAGER_H
#define OPTIONSMANAGER_H

#include <QDir>
#include <QFile>
#include <QTimer>
#include <QPointer>
#include <definitions/actiongroups.h>
#include <definitions/commandline.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/customborder.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/version.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/imacintegration.h>
#include <utils/log.h>
#include <utils/action.h>
#include <utils/widgetmanager.h>
#include <utils/customborderstorage.h>
#include <thirdparty/qtlockedfile/qtlockedfile.h>
#include "logindialog.h"
#include "optionswidget.h"
#include "optionsheader.h"
#include "optionsdialog.h"

class OptionsManager :
	public QObject,
	public IPlugin,
	public IOptionsManager,
	public IOptionsHolder
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IOptionsManager IOptionsHolder)
public:
	OptionsManager();
	~OptionsManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return OPTIONSMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IOptionsManager
	virtual bool isOpened() const;
	virtual QList<QString> profiles() const;
	virtual QString profilePath(const QString &AProfile) const;
	virtual QString lastActiveProfile() const;
	virtual QString currentProfile() const;
	virtual QByteArray currentProfileKey() const;
	virtual bool setCurrentProfile(const QString &AProfile, const QString &APassword);
	virtual QByteArray profileKey(const QString &AProfile, const QString &APassword) const;
	virtual QMap<QString, QVariant> profileData(const QString &AProfile) const;
	virtual bool setProfileData(const QString &AProfile, const QMap<QString, QVariant> &AData);
	virtual bool setProfileData(const QString &AProfile, const QString &AKey, const QVariant &AValue);
	virtual bool checkProfilePassword(const QString &AProfile, const QString &APassword) const;
	virtual bool changeProfilePassword(const QString &AProfile, const QString &AOldPassword, const QString &ANewPassword);
	virtual bool addProfile(const QString &AProfile, const QString &APassword);
	virtual bool renameProfile(const QString &AProfile, const QString &ANewName);
	virtual bool removeProfile(const QString &AProfile);
	virtual QList<QString> serverOptions() const;
	virtual void insertServerOption(const QString &APath);
	virtual void removeServerOption(const QString &APath);
	virtual QDialog *showLoginDialog(QWidget *AParent = NULL);
	virtual QList<IOptionsHolder *> optionsHolders() const;
	virtual void insertOptionsHolder(IOptionsHolder *AHolder);
	virtual void removeOptionsHolder(IOptionsHolder *AHolder);
	virtual QList<IOptionsDialogNode> optionsDialogNodes() const;
	virtual IOptionsDialogNode optionsDialogNode(const QString &ANodeId) const;
	virtual void insertOptionsDialogNode(const IOptionsDialogNode &ANode);
	virtual void removeOptionsDialogNode(const QString &ANodeId);
	virtual QWidget *showOptionsDialog(const QString &ANodeId = QString::null, QWidget *AParent = NULL);
	virtual IOptionsWidget *optionsHeaderWidget(const QString &AIconKey, const QString &ACaption, QWidget *AParent) const;
	virtual IOptionsWidget *optionsNodeWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) const;
signals:
	void profileAdded(const QString &AProfile);
	void profileOpened(const QString &AProfile);
	void profileClosed(const QString &AProfile);
	void profileRenamed(const QString &AProfile, const QString &ANewName);
	void profileRemoved(const QString &AProfile);
	void optionsHolderInserted(IOptionsHolder *AHolder);
	void optionsHolderRemoved(IOptionsHolder *AHolder);
	void optionsDialogNodeInserted(const IOptionsDialogNode &ANode);
	void optionsDialogNodeRemoved(const IOptionsDialogNode &ANode);
protected:
	void openProfile(const QString &AProfile, const QString &APassword);
	bool saveProfile(const QString &AProfile, const QDomDocument &AProfileDoc) const;
	void closeProfile();
	bool saveOptions() const;
	bool loadServerOptions(const Jid &AStreamJid);
	bool saveServerOptions(const Jid &AStreamJid);
	QDomDocument profileDocument(const QString &AProfile) const;
protected slots:
	void onOptionsChanged(const OptionsNode &ANode);
	void onOptionsDialogApplied();
	void onOptionsDialogDestroyed();
	void onChangeProfileByAction(bool);
	void onShowOptionsDialogByAction(bool);
	void onLoginDialogRejected();
	void onLoginDialogAccepted();
	void onAutoSaveTimerTimeout();
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageAboutToClose(const Jid &AStreamJid);
	void onAboutToQuit();
private:
	IPluginManager *FPluginManager;
	ITrayManager *FTrayManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IPrivateStorage *FPrivateStorage;
	IMacIntegration * FMacIntegration;
private:
	QDir FProfilesDir;
	QTimer FAutoSaveTimer;
private:
	QString FProfile;
	QByteArray FProfileKey;
	QDomDocument FProfileOptions;
	QtLockedFile *FProfileLocker;
private:
	QList<QString> FServerOptions;
private:
	Action *FChangeProfileAction;
	LoginDialog* FLoginDialog;
	CustomBorderContainer * FLoginDialogBorder;
private:
	Action *FShowOptionsDialogAction;
	QList<IOptionsHolder *> FOptionsHolders;
	QMap<QString, IOptionsDialogNode> FOptionsDialogNodes;
	OptionsDialog *FOptionsDialog;
	CustomBorderContainer *FOptionsDialogBorder;
};

#endif // OPTIONSMANAGER_H

#ifndef ICONNECTIONMANAGER_H
#define ICONNECTIONMANAGER_H

#include <QUuid>
#include <QDialog>
#include <QNetworkProxy>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>

#define CONNECTIONMANAGER_UUID      "{2f8ee0bf-4feb-420d-bd7c-ce1d2a910b5a}"

#define FIREFOX_PROXY_REF_UUID      "{baccbfb0-581d-4820-ac02-3980afd3969d}"
#define IEXPLORER_PROXY_REF_UUID    "{a99556f6-59ba-48eb-9021-22e9fa3ea95c}"
#define APPLICATION_PROXY_REF_UUID  "{6c456899-7a50-4cd1-b31d-3cbe49423ed1}"
#define MANUAL_PROXY_REF_UUID       "{5acc925d-9729-4997-9fbd-0771554bf94d}"

class IConnectionPlugin;

struct IConnectionProxy
{
	QString name;
	QNetworkProxy proxy;
};

class IConnection
{
public:
	virtual QObject *instance() =0;
	virtual IConnectionPlugin *ownerPlugin() const =0;
	virtual bool isOpen() const =0;
	virtual bool isEncrypted() const =0;
	virtual bool connectToHost() =0;
	virtual void disconnectFromHost() =0;
	virtual QString errorString() const =0;
	virtual qint64 write(const QByteArray &AData) =0;
	virtual QByteArray read(qint64 ABytes) =0;
protected:
	virtual void aboutToConnect() =0;
	virtual void connected() =0;
	virtual void encrypted() =0;
	virtual void readyRead(qint64 ABytes) =0;
	virtual void error(const QString &AError) =0;
	virtual void aboutToDisconnect() =0;
	virtual void disconnected() =0;
	virtual void connectionDestroyed() =0;
};

class IConnectionPlugin
{
public:
	virtual QObject *instance() =0;
	virtual QString pluginId() const =0;
	virtual QString pluginName() const =0;
	virtual IConnection *newConnection(const OptionsNode &ANode, QObject *AParent) =0;
	virtual IOptionsWidget *connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent) =0;
	virtual void saveConnectionSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual void loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode) =0;
protected:
	virtual void connectionCreated(IConnection *AConnection) =0;
	virtual void connectionDestroyed(IConnection *AConnection) =0;
};

class IConnectionManager
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> pluginList() const =0;
	virtual IConnectionPlugin *pluginById(const QString &APluginId) const =0;
	virtual QList<QUuid> proxyList() const =0;
	virtual IConnectionProxy proxyById(const QUuid &AProxyId) const =0;
	virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
	virtual void removeProxy(const QUuid &AProxyId) =0;
	virtual QUuid defaultProxy() const =0;
	virtual void setDefaultProxy(const QUuid &AProxyId) =0;
	virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL) =0;
	virtual IOptionsWidget *proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent) =0;
	virtual void saveProxySettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual QUuid loadProxySettings(const OptionsNode &ANode) const =0;
protected:
	virtual void connectionCreated(IConnection *AConnection) =0;
	virtual void connectionDestroyed(IConnection *AConnection) =0;
	virtual void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
	virtual void proxyRemoved(const QUuid &AProxyId) =0;
	virtual void defaultProxyChanged(const QUuid &AProxyId) =0;
};

Q_DECLARE_INTERFACE(IConnection,"Virtus.Plugin.IConnection/1.0")
Q_DECLARE_INTERFACE(IConnectionPlugin,"Virtus.Plugin.IConnectionPlugin/1.0")
Q_DECLARE_INTERFACE(IConnectionManager,"Virtus.Plugin.IConnectionManager/1.0")

#endif

#ifndef ISOCKSSTREAMS_H
#define ISOCKSSTREAMS_H

#include <QIODevice>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <interfaces/idatastreamsmanager.h>

#define SOCKSSTREAMS_UUID       "{4ef90e90-4069-4265-8149-3a778132e060}"

class ISocksStream :
			public IDataStreamSocket
{
public:
	virtual QIODevice *instance() =0;
	virtual bool disableDirectConnection() const =0;
	virtual void setDisableDirectConnection(bool ADisable) =0;
	virtual QString forwardHost() const =0;
	virtual quint16 forwardPort() const =0;
	virtual void setForwardAddress(const QString &AHost, quint16 APort) =0;
	virtual QNetworkProxy networkProxy() const =0;
	virtual void setNetworkProxy(const QNetworkProxy &AProxy) =0;
	virtual QList<QString> proxyList() const =0;
	virtual void setProxyList(const QList<QString> &AProxyList) =0;
protected:
	virtual void propertiesChanged() =0;
};

class ISocksStreams :
			public IDataStreamMethod
{
public:
	virtual QObject *instance() =0;
	virtual quint16 listeningPort() const =0;
	virtual QString accountStreamProxy(const Jid &AStreamJid) const =0;
	virtual QNetworkProxy accountNetworkProxy(const Jid &AStreamJid) const =0;
	virtual QString connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const =0;
	virtual bool appendLocalConnection(const QString &AKey) =0;
	virtual void removeLocalConnection(const QString &AKey) =0;
protected:
	virtual void localConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket) =0;
};

Q_DECLARE_INTERFACE(ISocksStream,"Virtus.Plugin.ISocksStream/1.0")
Q_DECLARE_INTERFACE(ISocksStreams,"Virtus.Plugin.ISocksStreams/1.0")

#endif // ISOCKSSTREAMS_H

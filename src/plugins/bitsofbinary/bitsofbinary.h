#ifndef BITSOFBINARY_H
#define BITSOFBINARY_H

#include <QDir>
#include <QMap>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibitsofbinary.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/log.h>

class BitsOfBinary :
	public QObject,
	public IPlugin,
	public IBitsOfBinary,
	public IXmppStanzaHadler,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBitsOfBinary IXmppStanzaHadler IStanzaHandler IStanzaRequestOwner);
public:
	BitsOfBinary();
	~BitsOfBinary();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return BITSOFBINARY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IBitsOfBinary
	virtual QString contentIdentifier(const QByteArray &AData) const;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool hasBinary(const QString &AContentId) const;
	virtual bool loadBinary(const QString &AContentId, const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool loadBinary(const QString &AContentId, QString &AType, QByteArray &AData, quint64 &AMaxAge);
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge);
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge, Stanza &AStanza);
	virtual bool removeBinary(const QString &AContentId);
signals:
	void binaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge);
	void binaryError(const QString &AContentId, const QString &AError);
	void binaryRemoved(const QString &AContentId);
protected:
	QString contentFileName(const QString &AContentId) const;
protected slots:
	void onXmppStreamCreated(IXmppStream *AXmppStream);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
private:
	int FSHIRequest;
private:
	QDir FDataDir;
	QMap<QString, QString> FLoadRequests;
};

#endif // BITSOFBINARY_H

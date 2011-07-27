#ifndef VCARDPLUGIN_H
#define VCARDPLUGIN_H

#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/toolbargroups.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ivcard.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ibitsofbinary.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iavatars.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irosterchanger.h>
#include <utils/widgetmanager.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include "vcard.h"
#include "vcarddialog.h"
#include "simplevcarddialog.h"

struct VCardItem
{
	VCardItem() {
		vcard = NULL;
		locks = 0;
	}
	VCard *vcard;
	int locks;
};

class VCardPlugin :
	public QObject,
	public IPlugin,
	public IVCardPlugin,
	public IStanzaRequestOwner,
	public IXmppUriHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IVCardPlugin IStanzaRequestOwner IXmppUriHandler)
	friend class VCard;
public:
	VCardPlugin();
	~VCardPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return VCARD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin()  { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IVCardPlugin
	virtual QString vcardFileName(const Jid &AContactJid) const;
	virtual bool hasVCard(const Jid &AContactJid) const;
	virtual IVCard *vcard(const Jid &AContactJid);
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool publishVCard(IVCard *AVCard, const Jid &AStreamJid);
	virtual void showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void showSimpleVCardDialog(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void vcardReceived(const Jid &AContactJid);
	void vcardPublished(const Jid &AContactJid);
	void vcardError(const Jid &AContactJid, const QString &AError);
	void avatarsRecieved(const Jid &AContactJid);
	void avatarsError(const Jid &AContactJid, const QString &AError);
protected:
	void unlockVCard(const Jid &AContactJid);
	void saveVCardFile(const QDomElement &AElem, const Jid &AContactJid) const;
	void removeEmptyChildElements(QDomElement &AElem) const;
	void registerDiscoFeatures();
protected slots:
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onShowVCardDialogByAction(bool);
	void onVCardDialogDestroyed(QObject *ADialog);
	void onSimpleVCardDialogDestroyed(QObject *ADialog);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onBinaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IXmppUriQueries *FXmppUriQueries;
	IBitsOfBinary *FBitsOfBinary;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IAvatars *FAvatars;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRosterChanger *FRosterChanger;
private:
	QMap<Jid, VCardItem> FVCards;
	QMap<QString, Jid> FVCardRequestId;
	QMap<QString, QString> FVCardPublishId;
	QMap<QString, Stanza> FVCardPublishStanza;
	QMap<Jid, VCardDialog *> FVCardDialogs;
	QMap<Jid, SimpleVCardDialog *> FSimpleVCardDialogs;
	QMap<QString, Jid> FAvatarsRequestId;
	QMap<QString, Jid> FAvatarsBinaryCids;
};

#endif // VCARDPLUGIN_H

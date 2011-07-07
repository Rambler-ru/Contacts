#ifndef AVATARS_H
#define AVATARS_H

#include <QDir>
#include <QTimer>
#include <QImageReader>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/vcardvaluenames.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ivcard.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imetacontacts.h>
#include <utils/options.h>
#include <utils/iconstorage.h>

struct AnimateAvatarParams
{
	AnimateAvatarParams() { timer = new QTimer; reader = NULL; }
	~AnimateAvatarParams() { timer->deleteLater(); delete reader; }
	int frameIndex;
	QTimer *timer;
	QImageReader *reader;
};

struct AutoAvatarParams
{
	AutoAvatarParams() { animation = NULL; }
	~AutoAvatarParams() { delete animation; }
	Jid contact;
	QSize size;
	QString prop;
	AnimateAvatarParams *animation;
};

class Avatars :
	public QObject,
	public IPlugin,
	public IAvatars,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IRosterDataHolder,
	public IOptionsHolder
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IAvatars IStanzaHandler IRosterDataHolder IStanzaRequestOwner IOptionsHolder)
public:
	Avatars();
	~Avatars();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return AVATARTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AId);
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IAvatars
	virtual QString avatarFileName(const QString &AHash) const;
	virtual bool hasAvatar(const QString &AHash) const;
	virtual QImage loadAvatar(const QString &AHash) const;
	virtual QString saveAvatar(const QByteArray &AImageData) const;
	virtual QString saveAvatar(const QImage &AImage, const char *AFormat = NULL) const;
	virtual QString avatarHash(const Jid &AContactJid) const;
	virtual QImage avatarImage(const Jid &AContactJid, bool AAllowNull = true, bool AAllowGray = true) const;
	virtual bool setAvatar(const Jid &AStreamJid, const QImage &AImage, const char *AFormat = NULL);
	virtual QString setCustomPictire(const Jid &AContactJid, const QString &AImageFile);
	virtual void insertAutoAvatar(QObject *AObject, const Jid &AContactJid, const QSize &ASize = QSize(), const QString &AProperty = "icon");
	virtual void removeAutoAvatar(QObject *AObject);
signals:
	void avatarChanged(const Jid &AContactJid);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	QByteArray loadAvatarFromVCard(const Jid &AContactJid) const;
	void updatePresence(const Jid &AStreamJid) const;
	void updateDataHolder(const Jid &AContactJid = Jid());
	bool updateVCardAvatar(const Jid &AContactJid, const QString &AHash, bool AFromVCard);
	bool updateIqAvatar(const Jid &AContactJid, const QString &AHash);
	void updateAvatarObject(QObject *AObject);
	void updateAutoAvatar(const Jid &AContactJid);
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onVCardChanged(const Jid &AContactJid);
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void onSetAvatarByAction(bool);
	void onClearAvatarByAction(bool);
	void onIconStorageChanged();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
	void onAvatarObjectTimerTimeout();
	void onAvatarObjectDestroyed(QObject *AObject);
	void onContactStateChanged(const Jid & AStreamJid, const Jid & AContactJid, bool AStateOnline);
	void onStreamStateChanged(const Jid & AStreamJid, bool AStateOnline);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IVCardPlugin *FVCardPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IOptionsManager *FOptionsManager;
private:
	QMap<Jid, int> FSHIPresenceIn;
	QMap<Jid, int> FSHIPresenceOut;
	QHash<Jid, QString> FVCardAvatars;
	QMultiMap<Jid, Jid> FBlockingResources;
private:
	QMap<Jid, int> FSHIIqAvatarIn;
	QHash<Jid, QString> FIqAvatars;
	QMap<QString, Jid> FIqAvatarRequests;
private:
	bool FAvatarsVisible;
	bool FShowEmptyAvatars;
	QMap<Jid, QString> FCustomPictures;
private:
	int FRosterLabelId;
	QDir FAvatarsDir;
	QImage FEmptyMaleAvatar;
	QImage FEmptyFemaleAvatar;
	QImage FEmptyMaleAvatarBig;
	QImage FEmptyFemaleAvatarBig;
	QImage FEmptyMaleAvatarOffline;
	QImage FEmptyFemaleAvatarOffline;
	QMap<Jid, QString> FStreamAvatars;
	mutable QHash<Jid, bool> FContactGender;
	mutable QHash<QString, QImage> FAvatarImages;
	mutable QHash<QString, QImage> FAvatarImagesGrayscale;
	QHash<QObject *, AutoAvatarParams> FAutoAvatars;
};

#endif // AVATARS_H

#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/dataformtypes.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iregistraton.h>
#include <interfaces/idataforms.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresence.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include "registerdialog.h"
#include "registerstream.h"

class Registration :
	public QObject,
	public IPlugin,
	public IRegistration,
	public IStanzaRequestOwner,
	public IXmppUriHandler,
	public IDiscoFeatureHandler,
	public IXmppFeaturesPlugin,
	public IOptionsHolder,
	public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRegistration IOptionsHolder IStanzaRequestOwner IXmppUriHandler IDiscoFeatureHandler IXmppFeaturesPlugin IDataLocalizer);
public:
	Registration();
	~Registration();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return REGISTRATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const { return QList<QString>() << NS_FEATURE_REGISTER; }
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//IRegistration
	virtual QString sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword);
	virtual QString sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit);
	virtual bool showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL);
signals:
	//IXmppFeaturesPlugin
	void featureCreated(IXmppFeature *AStreamFeature);
	void featureDestroyed(IXmppFeature *AStreamFeature);
	//IRegistration
	void registerFields(const QString &AId, const IRegisterFields &AFields);
	void registerSuccess(const QString &AId);
	void registerError(const QString &AId, const QString &ACondition, const QString &AMessage);
protected:
	void registerDiscoFeatures();
protected slots:
	void onRegisterActionTriggered(bool);
	void onXmppFeatureDestroyed();
private:
	IDataForms *FDataForms;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IPresencePlugin *FPresencePlugin;
	IOptionsManager *FOptionsManager;
	IAccountManager *FAccountManager;
	IXmppUriQueries *FXmppUriQueries;
private:
	QList<QString> FSendRequests;
	QList<QString> FSubmitRequests;
};

#endif // REGISTRATION_H

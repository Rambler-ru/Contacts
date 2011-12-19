#include "chatstates.h"

#include <QSet>
#include <QDateTime>
#include <QDataStream>

#define SHC_CONTENT_MESSAGES      "/message[@type='chat']/body"
#define SHC_STATE_MESSAGES        "/message/[@xmlns='" NS_CHATSTATES "']"

#define STATE_ACTIVE              "active"
#define STATE_COMPOSING           "composing"
#define STATE_PAUSED              "paused"
#define STATE_INACTIVE            "inactive"
#define STATE_GONE                "gone"

#define SFV_MAY_SEND              "may"
#define SFV_MUSTNOT_SEND          "mustnot"

#define PAUSED_TIMEOUT            30
#define INACTIVE_TIMEOUT          2*60
#define GONE_TIMEOUT              10*60

ChatStates::ChatStates()
{
	FPresencePlugin = NULL;
	FMessageWidgets = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;

	FUpdateTimer.setSingleShot(false);
	FUpdateTimer.setInterval(5000);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateSelfStates()));
}

ChatStates::~ChatStates()
{

}

void ChatStates::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Chat State Notifications");
	APluginInfo->description = tr("Allows to share information about the user's activity in the chat");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(PRESENCE_UUID);
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ChatStates::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowDestroyed(IChatWindow *)),SLOT(onChatWindowDestroyed(IChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceClosed(IPresence *)),SLOT(onPresenceClosed(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FPresencePlugin!=NULL && FMessageWidgets!=NULL && FStanzaProcessor!=NULL;
}

bool ChatStates::initObjects()
{
	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_CHATSTATE;
		notifyType.kindMask = INotification::RosterNotify|INotification::TabPageNotify;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_CHATSTATE_TYPING,notifyType);
	}
	return true;
}

bool ChatStates::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_CHATSTATESENABLED,true);
	return true;
}

bool ChatStates::startPlugin()
{
	FUpdateTimer.start();
	return true;
}

bool ChatStates::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessagesOut.value(AStreamJid)==AHandlerId && FChatParams.contains(AStreamJid) && AStanza.type()!="error")
	{
		bool stateSent = false;
		Jid contactJid = AStanza.to();
		if (isSupported(AStreamJid,contactJid))
		{
			IChatWindow *window = FMessageWidgets ? FMessageWidgets->findChatWindow(AStreamJid,contactJid) : NULL;
			if (window)
			{
				stateSent = true;
				AStanza.addElement(STATE_ACTIVE,NS_CHATSTATES);
			}
		}
		FChatParams[AStreamJid][contactJid].canSendStates = stateSent;
		setSelfState(AStreamJid,contactJid,IChatStates::StateActive,false);
	}
	else if (FSHIMessagesIn.value(AStreamJid)==AHandlerId && FChatParams.contains(AStreamJid) && AStanza.type()!="error")
	{
		Jid contactJid = AStanza.from();
		bool hasBody = !AStanza.firstElement("body").isNull();
		QDomElement elem = AStanza.firstElement(QString::null,NS_CHATSTATES);
		if (!elem.isNull())
		{
			if (hasBody || FChatParams.value(AStreamJid).value(contactJid).canSendStates)
			{
				AAccept = true;
				setSupported(AStreamJid,contactJid,true);
				FChatParams[AStreamJid][contactJid].canSendStates = true;

				int state = IChatStates::StateUnknown;
				if (elem.tagName() == STATE_ACTIVE)
					state = IChatStates::StateActive;
				else if (elem.tagName() == STATE_COMPOSING)
					state = IChatStates::StateComposing;
				else if (elem.tagName() == STATE_PAUSED)
					state = IChatStates::StatePaused;
				else if (elem.tagName() == STATE_INACTIVE)
					state = IChatStates::StateInactive;
				else if (elem.tagName() == STATE_GONE)
					state = IChatStates::StateGone;
				setUserState(AStreamJid,contactJid,state);
			}
		}
		else if (hasBody)
		{
			if (userChatState(AStreamJid,contactJid) != IChatStates::StateUnknown)
				setUserState(AStreamJid,contactJid,IChatStates::StateUnknown);
			setSupported(AStreamJid,contactJid,false);
		}
		return !hasBody;
	}
	return false;
}

int ChatStates::permitStatus(const Jid &AContactJid) const
{
	return FPermitStatus.value(AContactJid.bare(),IChatStates::StatusDefault);
}

void ChatStates::setPermitStatus(const Jid AContactJid, int AStatus)
{
	if (permitStatus(AContactJid) != AStatus)
	{
		bool oldEnabled = isEnabled(Jid(),AContactJid);

		Jid bareJid = AContactJid.bare();
		if (AStatus==IChatStates::StatusDisable)
		{
			FPermitStatus.insert(bareJid,AStatus);
		}
		else if (AStatus==IChatStates::StatusEnable)
		{
			FPermitStatus.insert(bareJid,AStatus);
		}
		else
		{
			FPermitStatus.remove(bareJid);
		}

		if (!oldEnabled && isEnabled(Jid(),AContactJid))
			resetSupported(AContactJid);

		emit permitStatusChanged(bareJid,AStatus);
	}
}

bool ChatStates::isEnabled(const Jid &AStreamJid, const Jid &AContactJid) const
{
	Q_UNUSED(AStreamJid);
	int status = permitStatus(AContactJid);
	return (Options::node(OPV_MESSAGES_CHATSTATESENABLED).value().toBool() || status==IChatStates::StatusEnable) && status!=IChatStates::StatusDisable;
}

bool ChatStates::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	bool supported = !FNotSupported.value(AStreamJid).contains(AContactJid);
	if (FDiscovery && supported && userChatState(AStreamJid,AContactJid)==IChatStates::StateUnknown)
		supported = !FDiscovery->hasDiscoInfo(AStreamJid, AContactJid) || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_CHATSTATES);
	return supported;
}

int ChatStates::userChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FChatParams.value(AStreamJid).value(AContactJid).userState;
}

int ChatStates::selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FChatParams.value(AStreamJid).value(AContactJid).selfState;
}

bool ChatStates::isSendingPossible(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return isEnabled(AStreamJid,AContactJid) && isSupported(AStreamJid,AContactJid) && FChatParams.value(AStreamJid).value(AContactJid).canSendStates;
}

void ChatStates::sendStateMessage(const Jid &AStreamJid, const Jid &AContactJid, int AState) const
{
	if (FStanzaProcessor && isSendingPossible(AStreamJid,AContactJid))
	{
		QString state;
		if (AState == IChatStates::StateActive)
			state = STATE_ACTIVE;
		else if (AState == IChatStates::StateComposing)
			state = STATE_COMPOSING;
		else if (AState == IChatStates::StatePaused)
			state = STATE_PAUSED;
		else if (AState == IChatStates::StateInactive)
			state = STATE_INACTIVE;
		else if (AState == IChatStates::StateGone)
			state = STATE_GONE;

		if (!state.isEmpty())
		{
			Stanza stanza("message");
			stanza.setType("chat").setTo(AContactJid.eFull());
			stanza.addElement(state,NS_CHATSTATES);
			FStanzaProcessor->sendStanzaOut(AStreamJid,stanza);
		}
	}
}

void ChatStates::resetSupported(const Jid &AContactJid)
{
	foreach (Jid streamJid, FNotSupported.keys())
		foreach (Jid contactJid, FNotSupported.value(streamJid))
			if (AContactJid.isEmpty() || (AContactJid && contactJid))
	setSupported(streamJid,contactJid,true);
}

void ChatStates::setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported)
{
	if (FNotSupported.contains(AStreamJid))
	{
		QList<Jid> &notSuppotred = FNotSupported[AStreamJid];
		int index = notSuppotred.indexOf(AContactJid);
		if (ASupported != (index<0))
		{
			if (ASupported)
			{
				notSuppotred.removeAt(index);
			}
			else
			{
				notSuppotred.append(AContactJid);
			}
			emit supportStatusChanged(AStreamJid,AContactJid,ASupported);
		}
	}
}

void ChatStates::setUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState)
{
	if (FChatParams.contains(AStreamJid))
	{
		ChatParams &params = FChatParams[AStreamJid][AContactJid];
		if (params.userState != AState)
		{
			params.userState = AState;
			emit userChatStateChanged(AStreamJid,AContactJid,AState);
			notifyUserState(AStreamJid,AContactJid);
		}
	}
}

void ChatStates::setSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend)
{
	if (FChatParams.contains(AStreamJid))
	{
		ChatParams &params = FChatParams[AStreamJid][AContactJid];
		params.selfLastActive = QDateTime::currentDateTime().toTime_t();
		if (params.selfState != AState)
		{
			params.selfState = AState;
			if (ASend)
				sendStateMessage(AStreamJid,AContactJid,AState);
			emit selfChatStateChanged(AStreamJid,AContactJid,AState);
		}
	}
}

void ChatStates::notifyUserState(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FNotifications)
	{
		ChatParams &params = FChatParams[AStreamJid][AContactJid];
		if (params.userState==IChatStates::StateComposing && params.notifyId<=0)
		{
			INotification notify;
			notify.kinds = FNotifications->notificationKinds(NNT_CHATSTATE_TYPING);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_CHATSTATE_TYPING;
				notify.data.insert(NDR_STREAM_JID, AStreamJid.full());
				notify.data.insert(NDR_CONTACT_JID, AContactJid.full());
				notify.data.insert(NDR_ICON_KEY, MNI_CHATSTATES_COMPOSING);
				notify.data.insert(NDR_ICON_STORAGE, RSR_STORAGE_MENUICONS);
				notify.data.insert(NDR_ROSTER_ORDER,RNO_CHATSTATES_TYPING);

				IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AContactJid) : NULL;
				if (window)
				{
					notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
					notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_CHATSTATE_TYPING);
					notify.data.insert(NDR_TABPAGE_ICONBLINK,false);
					notify.data.insert(NDR_TABPAGE_TOOLTIP,tr("Typing..."));
				}

				params.notifyId = FNotifications->appendNotification(notify);
			}
		}
		else if (params.userState!=IChatStates::StateComposing && params.notifyId>0)
		{
			FNotifications->removeNotification(params.notifyId);
			params.notifyId = 0;
		}
	}
}

void ChatStates::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.var = NS_CHATSTATES;
	dfeature.active = true;
	dfeature.name = tr("Chat State Notifications");
	dfeature.description = tr("Supports the exchanging of the information about the user's activity in the chat");
	FDiscovery->insertDiscoFeature(dfeature);
}

void ChatStates::onPresenceOpened(IPresence *APresence)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = APresence->streamJid();

		shandle.direction = IStanzaHandle::DirectionOut;
		shandle.order = SHO_MO_CHATSTATES;
		shandle.conditions.append(SHC_CONTENT_MESSAGES);
		FSHIMessagesOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.order = SHO_MI_CHATSTATES;
		shandle.conditions.append(SHC_STATE_MESSAGES);
		FSHIMessagesIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	FNotSupported[APresence->streamJid()].clear();
	FChatParams[APresence->streamJid()].clear();
}

void ChatStates::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
	{
		if (userChatState(APresence->streamJid(),AItem.itemJid) != IChatStates::StateUnknown)
			setUserState(APresence->streamJid(),AItem.itemJid,IChatStates::StateGone);
	}
}

void ChatStates::onPresenceClosed(IPresence *APresence)
{
	foreach(Jid contactJid, FChatParams.value(APresence->streamJid()).keys())
	{
		setUserState(APresence->streamJid(),contactJid,IChatStates::StateUnknown);
		setSelfState(APresence->streamJid(),contactJid,IChatStates::StateUnknown,false);
	}

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMessagesIn.take(APresence->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessagesOut.take(APresence->streamJid()));
	}

	FNotSupported.remove(APresence->streamJid());
	FChatParams.remove(APresence->streamJid());
}

void ChatStates::onChatWindowCreated(IChatWindow *AWindow)
{
	FChatByEditor.insert(AWindow->editWidget()->textEdit(),AWindow);
	connect(AWindow->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
	connect(AWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onChatWindowTextChanged()));
	connect(AWindow->instance(),SIGNAL(tabPageClosed()),SLOT(onChatWindowClosed()));
}

void ChatStates::onChatWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		int state = selfChatState(window->streamJid(),window->contactJid());
		if (state==IChatStates::StateUnknown || state==IChatStates::StateInactive || state==IChatStates::StateGone)
			setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onChatWindowTextChanged()
{
	QTextEdit *editor = qobject_cast<QTextEdit *>(sender());
	IChatWindow *window = FChatByEditor.value(editor,NULL);
	if (editor && window)
	{
		if (!editor->document()->isEmpty())
			setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
		else
			setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onChatWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		int state = selfChatState(window->streamJid(),window->contactJid());
		if (state != IChatStates::StateGone)
			setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
	}
}

void ChatStates::onChatWindowDestroyed(IChatWindow *AWindow)
{
	setSelfState(AWindow->streamJid(),AWindow->contactJid(),IChatStates::StateGone);
	FChatByEditor.remove(AWindow->editWidget()->textEdit());
}

void ChatStates::onUpdateSelfStates()
{
	QList<IChatWindow *> windows = FMessageWidgets!=NULL ? FMessageWidgets->chatWindows() : QList<IChatWindow *>();
	foreach (IChatWindow *window, windows)
	{
		if (FChatParams.value(window->streamJid()).contains(window->contactJid()))
		{
			ChatParams &params = FChatParams[window->streamJid()][window->contactJid()];
			uint timePassed = QDateTime::currentDateTime().toTime_t() - params.selfLastActive;
			if (params.selfState==IChatStates::StateActive && window->isActiveTabPage())
			{
				setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
			}
			else if (params.selfState==IChatStates::StateComposing && timePassed>PAUSED_TIMEOUT)
			{
				setSelfState(window->streamJid(),window->contactJid(),IChatStates::StatePaused);
			}
			else if ((params.selfState==IChatStates::StateActive || params.selfState==IChatStates::StatePaused) && timePassed>INACTIVE_TIMEOUT)
			{
				setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
			}
			else if (params.selfState==IChatStates::StateInactive && timePassed>GONE_TIMEOUT)
			{
				setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateGone);
			}
		}
	}
}

void ChatStates::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.chatstates.permit-status").toByteArray();
	QDataStream stream(data);
	stream >> FPermitStatus;
	onOptionsChanged(Options::node(OPV_MESSAGES_CHATSTATESENABLED));
}

void ChatStates::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FPermitStatus;
	Options::setFileValue(data,"messages.chatstates.permit-status");
}

void ChatStates::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_CHATSTATESENABLED)
	{
		if (ANode.value().toBool())
			resetSupported();
	}
}

Q_EXPORT_PLUGIN2(plg_chatstates, ChatStates)

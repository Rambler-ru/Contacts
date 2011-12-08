#include "chatmessagehandler.h"

#define HISTORY_TIME_PAST         5
#define HISTORY_MESSAGES_COUNT    25

#define DESTROYWINDOW_TIMEOUT     30*60*1000
#define CONSECUTIVE_TIMEOUT       2*60

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_TAB_PAGE_ID           Action::DR_Parametr2

#define URL_SCHEME_ACTION         "action"
#define URL_PATH_HISTORY          "history"
#define URL_PATH_CONTENT          "content"

#ifdef DEBUG_ENABLED
# include <QDebug>
#endif

QDataStream &operator<<(QDataStream &AStream, const TabPageInfo &AInfo)
{
	AStream << AInfo.streamJid;
	AStream << AInfo.contactJid;
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, TabPageInfo &AInfo)
{
	AStream >> AInfo.streamJid;
	AStream >> AInfo.contactJid;
	AInfo.page = NULL;
	return AStream;
}

ChatMessageHandler::ChatMessageHandler()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FMessageStyles = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRamblerHistory = NULL;
	FRostersView = NULL;
	FRostersModel = NULL;
	FAvatars = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FXmppUriQueries = NULL;
	FNotifications = NULL;
	FMetaContacts = NULL;
}

ChatMessageHandler::~ChatMessageHandler()
{

}

void ChatMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Chat Messages");
	APluginInfo->description = tr("Allows to exchange chat messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://contacts.rambler.ru";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
	APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}


bool ChatMessageHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
	{
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
		if (FMessageStyles)
		{
			connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
				SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRamblerHistory").value(0,NULL);
	if (plugin)
	{
		FRamblerHistory = qobject_cast<IRamblerHistory *>(plugin->instance());
		if (FRamblerHistory)
		{
			connect(FRamblerHistory->instance(),SIGNAL(serverMessagesLoaded(const QString &, const IHistoryMessages &)),
				SLOT(onRamblerHistoryMessagesLoaded(const QString &, const IHistoryMessages &)));
			connect(FRamblerHistory->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
				SLOT(onRamblerHistoryRequestFailed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationTest(const QString &, ushort)),
				SLOT(onNotificationTest(const QString &, ushort)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)),
				SLOT(onRosterIndexContextMenu(IRosterIndex *, QList<IRosterIndex *>, Menu *)));
			connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &, ToolBarChanger *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0,NULL);
	if (plugin)
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool ChatMessageHandler::initObjects()
{
	if (FMessageWidgets)
	{
		FMessageWidgets->insertTabPageHandler(this);
	}
	if (FRostersView)
	{
		FRostersView->insertClickHooker(RCHO_DEFAULT,this);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_CHATMESSAGEHANDLER,this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = OWO_NOTIFICATIONS_CHAT_MESSAGES;
		notifyType.title = tr("New messages");
		notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::TabPageNotify|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_CHAT_MESSAGE,notifyType);
	}
	return true;
}

bool ChatMessageHandler::tabPageAvail(const QString &ATabPageId) const
{
	if (FTabPages.contains(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IPresence *presence = findPresence(pageInfo.streamJid);
		if (presence)
		{
			IMetaRoster *mroster = FMetaContacts!=NULL ? FMetaContacts->findMetaRoster(presence->streamJid()) : NULL;
			if (mroster==NULL || !mroster->isEnabled())
			{
				if (pageInfo.page == NULL)
				{
					IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(presence->streamJid()) : NULL;
					return roster!=NULL && roster->rosterItem(pageInfo.contactJid).isValid;
				}
				return true;
			}
		}
	}
	return false;
}

ITabPage *ChatMessageHandler::tabPageFind(const QString &ATabPageId) const
{
	return FTabPages.contains(ATabPageId) ? FTabPages.value(ATabPageId).page : NULL;
}

ITabPage *ChatMessageHandler::tabPageCreate(const QString &ATabPageId)
{
	ITabPage *page = tabPageFind(ATabPageId);
	if (page==NULL && tabPageAvail(ATabPageId))
	{
		TabPageInfo &pageInfo = FTabPages[ATabPageId];
		IPresence *presence = findPresence(pageInfo.streamJid);
		if (presence)
		{
			IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
			if (pitem.isValid)
				page = getWindow(presence->streamJid(), pitem.itemJid);
			else
				page = getWindow(presence->streamJid(), pageInfo.contactJid.bare());
			pageInfo.page = page;
		}
	}
	return page;
}

Action *ChatMessageHandler::tabPageAction(const QString &ATabPageId, QObject *AParent)
{
	if (tabPageAvail(ATabPageId))
	{
		const TabPageInfo &pageInfo = FTabPages.value(ATabPageId);
		IPresence *presence = findPresence(pageInfo.streamJid);
		if (presence && presence->isOpen())
		{
			Action *action = new Action(AParent);
			action->setData(ADR_TAB_PAGE_ID, ATabPageId);
			action->setText(FNotifications!=NULL ? FNotifications->contactName(presence->streamJid(),pageInfo.contactJid) : pageInfo.contactJid.bare());
			connect(action,SIGNAL(triggered(bool)),SLOT(onOpenTabPageAction(bool)));

			ITabPage *page = tabPageFind(ATabPageId);
			if (page)
			{
				if (page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
				{
					ITabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
					if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
						action->setIcon(notify.iconStorage, notify.iconKey);
					else
						action->setIcon(notify.icon);
				}
				else
				{
					action->setIcon(page->tabPageIcon());
				}
			}
			else
			{
				IPresenceItem pitem = findPresenceItem(presence,pageInfo.contactJid);
				if (pitem.isValid)
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pitem.itemJid) : QIcon());
				else
					action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(presence->streamJid(),pageInfo.contactJid.bare()) : QIcon());
			}
			return action;
		}
	}
	return NULL;
}

bool ChatMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "message")
	{
		QString type = AParams.value("type");
		if (type == "chat")
		{
			IChatWindow *window = getWindow(AStreamJid, AContactJid);
			window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
			window->showTabPage();
			return true;
		}
	}
	return false;
}

bool ChatMessageHandler::rosterIndexClicked(IRosterIndex *AIndex, int AOrder)
{
	Q_UNUSED(AOrder);
	if (AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_MY_RESOURCE)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		return FMessageProcessor->createMessageWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
	return false;
}

bool ChatMessageHandler::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(ADirection);
	return !AMessage.body().isEmpty();
}

bool ChatMessageHandler::messageDisplay(const Message &AMessage, int ADirection)
{
	bool displayed = false;

	IChatWindow *window = NULL;
	if (ADirection == IMessageProcessor::MessageIn)
		window = AMessage.type()!=Message::Error ? getWindow(AMessage.to(),AMessage.from()) : findWindow(AMessage.to(),AMessage.from());
	else
		window = AMessage.type()!=Message::Error ? getWindow(AMessage.from(),AMessage.to()) : findWindow(AMessage.from(),AMessage.to());

	if (window && AMessage.type()!=Message::Error)
	{
		StyleExtension extension;
		WindowStatus &wstatus = FWindowStatus[window];
		if (ADirection==IMessageProcessor::MessageIn && !window->isActiveTabPage())
		{
			if (FDestroyTimers.contains(window))
				delete FDestroyTimers.take(window);
			extension.extensions = IMessageContentOptions::Unread;
		}
		extension.contentId = AMessage.data(MDR_STYLE_CONTENT_ID).toString();

		QUuid contentId = showStyledMessage(window,AMessage,extension);
		if (!contentId.isNull())
		{
			displayed = true;
			if (extension.extensions & IMessageContentOptions::Unread)
			{
				Message message = AMessage;
				message.setData(MDR_STYLE_CONTENT_ID,contentId.toString());
				wstatus.unread.append(message);
			}
		}

		if (wstatus.historyId.isNull() && FHistoryRequests.values().contains(window))
		{
			wstatus.pending.append(AMessage);
		}
	}
	else if (AMessage.type() == Message::Error)
	{
		LogError(QString("[ChatMessageHandler] Received error message:\n%1").arg(AMessage.stanza().toString()));
	}
	return displayed;
}

INotification ChatMessageHandler::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::MessageIn)
	{
		IChatWindow *window = getWindow(AMessage.to(),AMessage.from());
		if (!window->isActiveTabPage())
		{
			WindowStatus &wstatus = FWindowStatus[window];

			QString name = ANotifications->contactName(AMessage.to(),AMessage.from());
			QString messages = tr("%n message(s)","",wstatus.notified.count()+1);

			notify.kinds = ANotifications->notificationKinds(NNT_CHAT_MESSAGE);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_CHAT_MESSAGE;
				notify.data.insert(NDR_STREAM_JID,AMessage.to());
				notify.data.insert(NDR_CONTACT_JID,AMessage.from());
				notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
				notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
				notify.data.insert(NDR_ROSTER_ORDER,RNO_CHATHANDLER_MESSAGE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::ExpandParents);
				notify.data.insert(NDR_ROSTER_HOOK_CLICK,true);
				notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
				notify.data.insert(NDR_ROSTER_FOOTER,messages);
				notify.data.insert(NDR_ROSTER_BACKGROUND,QBrush(Qt::yellow));
				notify.data.insert(NDR_TRAY_TOOLTIP,QString("%1 - %2").arg(name.split(" ").value(0)).arg(messages));
				notify.data.insert(NDR_ALERT_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_NEW_MESSAGE);
				notify.data.insert(NDR_TABPAGE_ICONBLINK,true);
				notify.data.insert(NDR_TABPAGE_TOOLTIP,messages);
				notify.data.insert(NDR_TABPAGE_STYLEKEY,STS_CHATHANDLER_TABBARITEM_NEWMESSAGE);
				notify.data.insert(NDR_POPUP_ICON, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE));
				notify.data.insert(NDR_POPUP_TITLE,name);
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(AMessage.to(),AMessage.from()));
				notify.data.insert(NDR_SOUND_FILE,SDF_CHATHANDLER_MESSAGE);

				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AMessage.to()) : NULL;
				if (roster && !roster->rosterItem(AMessage.from()).isValid)
					notify.data.insert(NDR_POPUP_NOTICE,tr("Not in contact list"));

				wstatus.notified.append(AMessage.data(MDR_MESSAGE_ID).toInt());
				int notifyCount = wstatus.notified.count();
				if (notifyCount > 1)
				{
					int lastNotifyWithPopup = -1;
					QList<int> notifies = ANotifications->notifications();
					while (lastNotifyWithPopup<0 && !notifies.isEmpty())
					{
						int notifyId = notifies.takeLast();
						if ((ANotifications->notificationById(notifyId).kinds & INotification::PopupWindow) > 0)
							lastNotifyWithPopup = notifyId;
					}

					int replNotify = FMessageProcessor->notifyByMessage(wstatus.notified.value(wstatus.notified.count()-2));
					if (replNotify>0 && replNotify==lastNotifyWithPopup)
						notify.data.insert(NDR_REPLACE_NOTIFY, replNotify);
					else
						replNotify = -1;

					foreach(int messageId, wstatus.notified)
					{
						int notifyId = FMessageProcessor->notifyByMessage(messageId);
						if (notifyId>0 && notifyId!=replNotify)
							notifyCount -= FNotifications!=NULL ? FNotifications->notificationById(notifyId).data.value(NDR_TABPAGE_NOTIFYCOUNT).toInt() : 0;
					}
				}
				notify.data.insert(NDR_TABPAGE_NOTIFYCOUNT,notifyCount);

#ifdef Q_WS_MAC
				notify.data.insert(NDR_POPUP_TEXT,AMessage.body());
#else
				QTextDocument doc;
				FMessageProcessor->messageToText(&doc,AMessage);
				notify.data.insert(NDR_POPUP_TEXT,getHtmlBody(doc.toHtml()));
#endif

				updateWindow(window);
			}
		}
	}
	return notify;
}

bool ChatMessageHandler::messageShowWindow(int AMessageId)
{
	IChatWindow *window = findNotifiedMessageWindow(AMessageId);
	if (window)
	{
		window->showTabPage();
		return true;
	}
	return false;
}

bool ChatMessageHandler::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	if (AType == Message::Chat)
	{
		IChatWindow *window = getWindow(AStreamJid,AContactJid);
		if (window)
		{
			if (AShowMode == IMessageHandler::SM_SHOW)
				window->showTabPage();
			else if (AShowMode == IMessageHandler::SM_ASSIGN)
				window->assignTabPage();
			else if (AShowMode == IMessageHandler::SM_MINIMIZED)
				window->showMinimizedTabPage();
			return true;
		}
	}
	return false;
}

IChatWindow *ChatMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IChatWindow *window = NULL;
	if (AStreamJid.isValid() && AContactJid.isValid())
	{
		window = findWindow(AStreamJid,AContactJid,false);
		if (window == NULL)
		{
			window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
			if (window)
			{
				window->infoWidget()->autoUpdateFields();
				window->setTabPageNotifier(FMessageWidgets->newTabPageNotifier(window));

				WindowStatus &wstatus = FWindowStatus[window];
				wstatus.createTime = QDateTime::currentDateTime();

				connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
				connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
					SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
				connect(window->viewWidget()->instance(),SIGNAL(urlClicked(const QUrl	&)),SLOT(onUrlClicked(const QUrl	&)));
				connect(window->instance(),SIGNAL(tabPageClosed()),SLOT(onWindowClosed()));
				connect(window->instance(),SIGNAL(tabPageActivated()),SLOT(onWindowActivated()));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onWindowDestroyed()));

				FWindows.append(window);
				updateWindow(window);
				setMessageStyle(window);

				if (FRostersView && FRostersModel)
				{
					UserContextMenu *menu = new UserContextMenu(FRostersModel,FRostersView,window);
					if (FAvatars)
						FAvatars->insertAutoAvatar(menu->menuAction(),AContactJid,QSize(48,48));

					QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
					button->setPopupMode(QToolButton::InstantPopup);
					button->setFixedSize(QSize(48,48));
				}

				TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
				pageInfo.page = window;
				emit tabPageCreated(window);

				requestHistoryMessages(window, HISTORY_MESSAGES_COUNT);

				window->instance()->installEventFilter(this);
			}
		}
	}
	return window;
}

IChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExactMatch) const
{
	IChatWindow *bareWindow = NULL;
	foreach(IChatWindow *window,FWindows)
	{
		if (window->streamJid() == AStreamJid)
		{
			if (window->contactJid() == AContactJid)
				return window;
			else if (!AExactMatch && !bareWindow && (window->contactJid() && AContactJid))
				bareWindow = window;
		}
	}
	return bareWindow;
}

IChatWindow *ChatMessageHandler::findNotifiedMessageWindow(int AMessageId) const
{
	foreach(IChatWindow *window, FWindows)
		if (FWindowStatus.value(window).notified.contains(AMessageId))
			return window;
	return NULL;
}

void ChatMessageHandler::clearWindow(IChatWindow *AWindow)
{
	IMessageStyle *style = AWindow->viewWidget()!=NULL ? AWindow->viewWidget()->messageStyle() : NULL;
	if (style!=NULL)
	{
		IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
		style->changeOptions(AWindow->viewWidget()->styleWidget(),soptions,true);
		resetWindowStatus(AWindow);
	}
}

void ChatMessageHandler::updateWindow(IChatWindow *AWindow)
{
	QIcon icon;
	if (!FWindowStatus.value(AWindow).notified.isEmpty())
		icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
	else if (FStatusIcons)
		icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

	QString name = FMetaContacts!=NULL ? FMetaContacts->itemHint(AWindow->contactJid()) : AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
	QString show = FStatusChanger ? FStatusChanger->nameByShow(AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt()) : QString::null;
	//QString title = name + (!show.isEmpty() ? QString(" (%1)").arg(show) : QString::null);
	QString title = name;
	AWindow->updateWindow(icon,name,title,show);
}

void ChatMessageHandler::resetWindowStatus(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	wstatus.separators.clear();
	wstatus.unread.clear();
	wstatus.offline.clear();
	wstatus.pending.clear();
	wstatus.historyId = QString::null;
	wstatus.historyTime = QDateTime();
	wstatus.historyContentId = QUuid();
	wstatus.lastStatusShow = QString::null;
}

void ChatMessageHandler::removeMessageNotifications(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.notified.isEmpty())
	{
		foreach(int messageId, wstatus.notified)
			FMessageProcessor->removeMessageNotify(messageId);
		wstatus.notified.clear();
		updateWindow(AWindow);
	}
}

void ChatMessageHandler::replaceUnreadMessages(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.unread.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		foreach (Message message, wstatus.unread)
		{
			extension.contentId = message.data(MDR_STYLE_CONTENT_ID).toString();
			showStyledMessage(AWindow, message, extension);
		}
		wstatus.unread.clear();
	}
}

void ChatMessageHandler::sendOfflineMessages(IChatWindow *AWindow)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.offline.isEmpty())
	{
		StyleExtension extension;
		extension.action = IMessageContentOptions::Replace;
		while (!wstatus.offline.isEmpty())
		{
			Message message = wstatus.offline.at(0);
			message.setTo(AWindow->contactJid().eFull());
			if (!FMessageProcessor->sendMessage(AWindow->streamJid(),message,IMessageProcessor::MessageOut))
			{
				LogError(QString("[ChatMessageHandler] Failed to send %1 stored offline messages").arg(wstatus.offline.count()));
				break;
			}
		}
	}
}

void ChatMessageHandler::removeOfflineMessage(IChatWindow *AWindow, const QUuid &AContentId)
{
	WindowStatus &wstatus = FWindowStatus[AWindow];
	for (int index=0; index<wstatus.offline.count(); index++)
	{
		if (wstatus.offline.at(index).data(MDR_STYLE_CONTENT_ID).toString() == AContentId.toString())
		{
			IMessageContentOptions options;
			options.action = IMessageContentOptions::Remove;
			options.contentId = AContentId;
			AWindow->viewWidget()->changeContentHtml(QString::null,options);
			wstatus.offline.removeAt(index);
			break;
		}
	}
}

void ChatMessageHandler::requestHistoryMessages(IChatWindow *AWindow, int ACount)
{
	if (FRamblerHistory && FRamblerHistory->isSupported(AWindow->streamJid()))
	{
		IHistoryRetrieve retrieve;
		retrieve.with = AWindow->contactJid();
		retrieve.beforeId = FWindowStatus.value(AWindow).historyId;
		retrieve.beforeTime = FWindowStatus.value(AWindow).historyTime;
		retrieve.count = ACount;
		QString id = FRamblerHistory->loadServerMessages(AWindow->streamJid(),retrieve);
		if (!id.isEmpty())
		{
			FHistoryRequests.insert(id,AWindow);
			showHistoryLinks(AWindow,HLS_WAITING);
		}
	}
}

IPresence *ChatMessageHandler::findPresence(const Jid &AStreamJid) const
{
	IPresence *precsence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	for (int i=0; precsence==NULL && i<FPrecences.count(); i++)
		if (AStreamJid && FPrecences.at(i)->streamJid())
			precsence = FPrecences.at(i);
	return precsence;
}

IPresenceItem ChatMessageHandler::findPresenceItem(IPresence *APresence, const Jid &AContactJid) const
{
	IPresenceItem pitem = APresence!=NULL ? APresence->presenceItem(AContactJid) : IPresenceItem();
	QList<IPresenceItem> pitems = APresence!=NULL ? APresence->presenceItems() : QList<IPresenceItem>();
	for (int i=0; !pitem.isValid && i<pitems.count(); i++)
		if (AContactJid && pitems.at(i).itemJid)
			pitem = pitems.at(i);
	return pitem;
}

void ChatMessageHandler::showHistoryLinks(IChatWindow *AWindow, HisloryLoadState AState)
{
	static QString urlMask = QString("<a class=\"%3\" href='%1'>%2</a>");
	static QString msgMask = QString("<div class=\"%2\">%1</div>");

	if (FRamblerHistory && FRamblerHistory->isSupported(AWindow->streamJid()))
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.time = QDateTime::fromTime_t(0);
		options.timeFormat = " ";
		options.noScroll = true;
		options.status = IMessageContentOptions::HistoryShow;

		QString message = "<div class=\"v-chat-header\">";

		if (AState == HLS_READY)
		{
			QUrl showMesagesUrl;
			showMesagesUrl.setScheme(URL_SCHEME_ACTION);
			showMesagesUrl.setPath(URL_PATH_HISTORY);
			showMesagesUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("messages")));
			message += urlMask.arg(showMesagesUrl.toString()).arg(tr("Show previous messages")).arg("v-chat-header-b");
		}
		else if (AState == HLS_WAITING)
		{
			message += msgMask.arg(tr("Loading messages from server...")).arg("v-chat-header-message");
		}
		else if (AState == HLS_FINISHED)
		{
			message += msgMask.arg(tr("All messages loaded")).arg("v-chat-header-message");
		}
		else if (AState == HLS_FAILED)
		{
			message += msgMask.arg(tr("Failed to load history messages from server")).arg("v-chat-header-message");
		}

#ifdef DEBUG_ENABLED
		QUrl updateHistoryUrl;
		updateHistoryUrl.setScheme(URL_SCHEME_ACTION);
		updateHistoryUrl.setPath(URL_PATH_HISTORY);
		updateHistoryUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("update")));
		message += urlMask.arg(updateHistoryUrl.toString()).arg(QString::null).arg("v-chat-header-b v-chat-header-reload");
#endif

		QUrl showWindowUrl;
		showWindowUrl.setScheme(URL_SCHEME_ACTION);
		showWindowUrl.setPath(URL_PATH_HISTORY);
		showWindowUrl.setQueryItems(QList< QPair<QString, QString> >() << qMakePair<QString,QString>(QString("show"),QString("window")));
		message += urlMask.arg(showWindowUrl.toString()).arg(tr("Chat history")).arg("v-chat-header-history");

		WindowStatus &wstatus = FWindowStatus[AWindow];
		if (!wstatus.historyContentId.isNull())
		{
			options.action = IMessageContentOptions::Replace;
			options.contentId = wstatus.historyContentId;
		}
		wstatus.historyContentId = AWindow->viewWidget()->changeContentHtml(message,options);
	}
}

void ChatMessageHandler::setMessageStyle(IChatWindow *AWindow)
{
	IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
	IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
	AWindow->viewWidget()->setMessageStyle(style,soptions);
	resetWindowStatus(AWindow);
	showHistoryLinks(AWindow, HLS_READY);
}

void ChatMessageHandler::fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
	if (AOptions.direction == IMessageContentOptions::DirectionIn)
	{
		AOptions.senderId = AWindow->contactJid().full();
		AOptions.senderName = Qt::escape(FMessageStyles->contactName(AWindow->streamJid(),AWindow->contactJid()));
		AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->contactJid());
		AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid(),AWindow->contactJid());
		AOptions.senderColor = "blue";
	}
	else
	{
		AOptions.senderId = AWindow->streamJid().full();
		if (AWindow->streamJid() && AWindow->contactJid())
			AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().node());
		else
			AOptions.senderName = Qt::escape(FMessageStyles->contactName(AWindow->streamJid()));
		AOptions.senderAvatar = FMessageStyles->contactAvatar(AWindow->streamJid());
		AOptions.senderIcon = FMessageStyles->contactIcon(AWindow->streamJid());
		AOptions.senderColor = "red";
	}
}

QUuid ChatMessageHandler::showDateSeparator(IChatWindow *AWindow, const QDate &ADate)
{
	static const QList<QString> mnames = QList<QString>() << tr("January") << tr("February") <<  tr("March") <<  tr("April")
		<< tr("May") << tr("June") << tr("July") << tr("August") << tr("September") << tr("October") << tr("November") << tr("December");
	static const QList<QString> dnames = QList<QString>() << tr("Monday") << tr("Tuesday") <<  tr("Wednesday") <<  tr("Thursday")
		<< tr("Friday") << tr("Saturday") << tr("Sunday");

	WindowStatus &wstatus = FWindowStatus[AWindow];
	if (!wstatus.separators.contains(ADate))
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::Status;
		options.status = IMessageContentOptions::DateSeparator;
		options.direction = IMessageContentOptions::DirectionIn;
		options.time.setDate(ADate);
		options.time.setTime(QTime(0,0));
		options.timeFormat = " ";
		options.noScroll = true;

		QString message;
		QDate currentDate = QDate::currentDate();
		if (ADate == currentDate)
			message = ADate.toString(tr("%1, %2 dd")).arg(tr("Today")).arg(mnames.value(ADate.month()-1));
		else if (ADate.year() == currentDate.year())
			message = ADate.toString(tr("%1, %2 dd")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		else
			message = ADate.toString(tr("%1, %2 dd, yyyy")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
		wstatus.separators.append(ADate);
		return AWindow->viewWidget()->changeContentText(message,options);
	}
	return QUuid();
}

QUuid ChatMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Status;
	options.time = QDateTime::currentDateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);
	options.direction = IMessageContentOptions::DirectionIn;
	fillContentOptions(AWindow,options);
	return AWindow->viewWidget()->changeContentText(AMessage,options);
}

QUuid ChatMessageHandler::showStyledMessage(IChatWindow *AWindow, const Message &AMessage, const StyleExtension &AExtension)
{
	IMessageContentOptions options;
	options.kind = IMessageContentOptions::Message;
	options.time = AMessage.dateTime();
	options.timeFormat = FMessageStyles->timeFormat(options.time);

	if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid() != AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
		options.direction = IMessageContentOptions::DirectionIn;
	else
		options.direction = IMessageContentOptions::DirectionOut;

	if (options.time.secsTo(FWindowStatus.value(AWindow).createTime) > HISTORY_TIME_PAST)
	{
		options.noScroll = true;
		options.type |= IMessageContentOptions::History;
	}

	options.action = AExtension.action;
	options.extensions = AExtension.extensions;
	options.contentId = AExtension.contentId;

	fillContentOptions(AWindow,options);
	showDateSeparator(AWindow,AMessage.dateTime().date());
	return AWindow->viewWidget()->changeContentMessage(AMessage,options);
}

bool ChatMessageHandler::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type()==QEvent::WindowDeactivate || AEvent->type()==QEvent::Hide)
	{
		IChatWindow *window = qobject_cast<IChatWindow *>(AObject);
		if (window)
			replaceUnreadMessages(window);
	}
	return QObject::eventFilter(AObject,AEvent);
}

void ChatMessageHandler::onMessageReady()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		Message message;
		message.setTo(window->contactJid().eFull()).setType(Message::Chat);
		FMessageProcessor->textToMessage(message,window->editWidget()->document());
		if (!message.body().isEmpty())
		{
			if (!FMessageProcessor->sendMessage(window->streamJid(),message,IMessageProcessor::MessageOut))
			{
				StyleExtension extension;
				extension.extensions = IMessageContentOptions::Offline;
				QUuid contentId = showStyledMessage(window, message, extension);
				if (!contentId.isNull())
				{
					message.setData(MDR_STYLE_CONTENT_ID, contentId.toString());
					FWindowStatus[window].offline.append(message);
				}
			}

			replaceUnreadMessages(window);
			window->editWidget()->clearEditor();
		}
	}
}

void ChatMessageHandler::onUrlClicked(const QUrl &AUrl)
{
	if (AUrl.scheme() == URL_SCHEME_ACTION)
	{
		IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			if (AUrl.path() == URL_PATH_HISTORY)
			{
				QString keyValue = AUrl.queryItemValue("show");
				if (keyValue == "messages")
				{
					requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
				}
				else if (keyValue == "window")
				{
					if (FRamblerHistory)
						FRamblerHistory->showViewHistoryWindow(window->streamJid(),window->contactJid());
				}
				else if (keyValue == "update")
				{
					clearWindow(window);
					requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
				}
			}
			else if (AUrl.path() == URL_PATH_CONTENT)
			{
				QUuid contentId = AUrl.queryItemValue("remove");
				if (!contentId.isNull())
				{
					removeOfflineMessage(window,contentId);
				}
			}
		}
	}
}

void ChatMessageHandler::onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
	Q_UNUSED(AValue);
	if ((AField & (IInfoWidget::ContactStatus|IInfoWidget::ContactName))>0)
	{
		IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
		IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
		if (window)
		{
			if (AField==IInfoWidget::ContactShow || AField==IInfoWidget::ContactStatus)
			{
				QString status = widget->field(IInfoWidget::ContactStatus).toString();
				QString show = FStatusChanger ? FStatusChanger->nameByShow(widget->field(IInfoWidget::ContactShow).toInt()) : QString::null;
				WindowStatus &wstatus = FWindowStatus[window];
				if (Options::node(OPV_MESSAGES_SHOWSTATUS).value().toBool() && wstatus.lastStatusShow!=status+show)
				{
					QString message = tr("%1 changed status to [%2] %3").arg(widget->field(IInfoWidget::ContactName).toString()).arg(show).arg(status);
					showStyledStatus(window,message);
				}
				wstatus.lastStatusShow = status+show;
			}
			updateWindow(window);
		}
	}
}

void ChatMessageHandler::onWindowActivated()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		TabPageInfo &pageInfo = FTabPages[window->tabPageId()];
		pageInfo.streamJid = window->streamJid();
		pageInfo.contactJid = window->contactJid();
		pageInfo.page = window;

		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		removeMessageNotifications(window);
	}
}

void ChatMessageHandler::onWindowClosed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window && FWindowStatus.value(window).notified.isEmpty())
	{
		if (!FDestroyTimers.contains(window))
		{
			QTimer *timer = new QTimer;
			timer->setSingleShot(true);
			connect(timer,SIGNAL(timeout()),window->instance(),SLOT(deleteLater()));
			FDestroyTimers.insert(window,timer);
		}
		FDestroyTimers[window]->start(DESTROYWINDOW_TIMEOUT);
	}
}

void ChatMessageHandler::onWindowDestroyed()
{
	IChatWindow *window = qobject_cast<IChatWindow *>(sender());
	if (window)
	{
		if (FTabPages.contains(window->tabPageId()))
			FTabPages[window->tabPageId()].page = NULL;
		if (FDestroyTimers.contains(window))
			delete FDestroyTimers.take(window);
		removeMessageNotifications(window);
		FWindows.removeAll(window);
		FWindowStatus.remove(window);
		emit tabPageDestroyed(window);
	}
}

void ChatMessageHandler::onStatusIconsChanged()
{
	foreach(IChatWindow *window, FWindows)
		updateWindow(window);
}

void ChatMessageHandler::onShowWindowAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		FMessageProcessor->createMessageWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
	}
}

void ChatMessageHandler::onOpenTabPageAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		ITabPage *page = tabPageCreate(action->data(ADR_TAB_PAGE_ID).toString());
		if (page)
			page->showTabPage();
	}
}

void ChatMessageHandler::onRosterIndexContextMenu(IRosterIndex *AIndex, QList<IRosterIndex *> ASelected, Menu *AMenu)
{
	static QList<int> chatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IPresence *presence = FPresencePlugin ? FPresencePlugin->findPresence(streamJid) : NULL;
	if (presence && presence->isOpen() && ASelected.count()<2)
	{
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		if (chatActionTypes.contains(AIndex->type()))
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Open"));
			action->setData(ADR_STREAM_JID,streamJid.full());
			action->setData(ADR_CONTACT_JID,contactJid.full());
			AMenu->setDefaultAction(action);
			AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER_OPENCHAT,true);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void ChatMessageHandler::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips, ToolBarChanger *AToolBarChanger)
{
	Q_UNUSED(AToolTips);
	static QList<int> chatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IPresence *presence = FPresencePlugin ? FPresencePlugin->findPresence(streamJid) : NULL;
	if (presence && presence->isOpen())
	{
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		if (AToolBarChanger && chatActionTypes.contains(AIndex->type()) && (ALabelId == RLID_DISPLAY))
		{
			Action *action = new Action(AToolBarChanger->toolBar());
			action->setText(tr("Open"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
			action->setData(ADR_STREAM_JID,streamJid.full());
			action->setData(ADR_CONTACT_JID,contactJid.full());
			AToolBarChanger->insertAction(action,TBG_RVLTT_CHATMESSAGEHANDLER);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
		}
	}
}

void ChatMessageHandler::onPresenceAdded(IPresence *APresence)
{
	FPrecences.append(APresence);
}

void ChatMessageHandler::onPresenceOpened(IPresence *APresence)
{
	foreach(IChatWindow *window, FWindows)
	{
		if (window->streamJid() == APresence->streamJid())
		{
			sendOfflineMessages(window);
			if (FRamblerHistory && FRamblerHistory->isSupported(window->streamJid()))
			{
				clearWindow(window);
				requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
			}
		}
	}
}

void ChatMessageHandler::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (!AItem.itemJid.resource().isEmpty() && AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error)
	{
		IChatWindow *fullWindow = findWindow(APresence->streamJid(),AItem.itemJid);

		IChatWindow *bareWindow = findWindow(APresence->streamJid(),AItem.itemJid.bare());
		if (bareWindow)
		{
			if (!fullWindow)
				bareWindow->setContactJid(AItem.itemJid);
			else if (FWindowStatus.value(bareWindow).notified.isEmpty())
				bareWindow->instance()->deleteLater();
		}

		if (!fullWindow && !bareWindow)
		{
			foreach(IChatWindow *offlineWindow, FWindows)
			{
				if (offlineWindow->streamJid()==APresence->streamJid() && (offlineWindow->contactJid() && AItem.itemJid))
				{
					int show = APresence->presenceItem(offlineWindow->contactJid()).show;
					if (show==IPresence::Offline || show==IPresence::Error)
					{
						offlineWindow->setContactJid(AItem.itemJid);
						break;
					}
				}
			}
		}
	}
}

void ChatMessageHandler::onPresenceRemoved(IPresence *APresence)
{
	FPrecences.removeAll(APresence);
}

void ChatMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
	if (AMessageType==Message::Chat && AContext.isEmpty())
	{
		foreach (IChatWindow *window, FWindows)
		{
			IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
			if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
			{
				setMessageStyle(window);
				requestHistoryMessages(window,HISTORY_MESSAGES_COUNT);
			}
		}
	}
}

void ChatMessageHandler::onNotificationTest(const QString &ATypeId, ushort AKinds)
{
	if (ATypeId == NNT_CHAT_MESSAGE)
	{
		INotification notify;
		notify.kinds = AKinds;
		notify.typeId = ATypeId;
		notify.flags |= INotification::TestNotify;
		if (AKinds & INotification::PopupWindow)
		{
			Jid contsctJid = "vasilisa@rambler/ramblercontacts";
			notify.data.insert(NDR_STREAM_JID,contsctJid.full());
			notify.data.insert(NDR_CONTACT_JID,contsctJid.full());
			notify.data.insert(NDR_ICON_KEY,MNI_CHAT_MHANDLER_MESSAGE);
			notify.data.insert(NDR_ICON_STORAGE,RSR_STORAGE_MENUICONS);
			notify.data.insert(NDR_POPUP_ICON, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE));
			notify.data.insert(NDR_POPUP_TITLE,tr("Vasilisa Premudraya"));
		 notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(Jid::null,contsctJid.full()));
			notify.data.insert(NDR_POPUP_TEXT,tr("Hi! Come on www.rambler.ru :)"));
		}
		if (AKinds & INotification::SoundPlay)
		{
			notify.data.insert(NDR_SOUND_FILE,SDF_CHATHANDLER_MESSAGE);
		}
		if (!notify.data.isEmpty())
		{
			FNotifications->appendNotification(notify);
		}
	}
}

void ChatMessageHandler::onRamblerHistoryMessagesLoaded(const QString &AId, const IHistoryMessages &AMessages)
{
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			QList<Message> historyMessages = AMessages.messages;

			bool found = false;
			WindowStatus &wstatus = FWindowStatus[window];
			while (!wstatus.pending.isEmpty() && !historyMessages.isEmpty())
			{
				Message pMessage = wstatus.pending.takeLast();
				if (Jid(pMessage.to()).pBare() == Jid(historyMessages.last().to()).pBare() &&
					pMessage.body() == historyMessages.last().body() &&
					qAbs(pMessage.dateTime().secsTo(historyMessages.last().dateTime()))<=3*60)
				{
					found = true;
					historyMessages.removeLast();
				}
				else if (found)
				{
					wstatus.pending.clear();
				}
			}
			wstatus.pending.clear();

			for (int i=0; i<historyMessages.count(); i++)
			{
				Message message = historyMessages.at(i);
				showStyledMessage(window,message);
			}

			if (!AMessages.beforeId.isEmpty())
			{
				FWindowStatus[window].historyId = AMessages.beforeId;
				FWindowStatus[window].historyTime = AMessages.beforeTime;
			}

			if (AMessages.messages.count() < HISTORY_MESSAGES_COUNT)
				showHistoryLinks(window,HLS_FINISHED);
			else
				showHistoryLinks(window,HLS_READY);
		}
	}
}

void ChatMessageHandler::onRamblerHistoryRequestFailed(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FHistoryRequests.contains(AId))
	{
		IChatWindow *window = FHistoryRequests.take(AId);
		if (FWindows.contains(window))
		{
			WindowStatus &wstatus = FWindowStatus[window];
			wstatus.pending.clear();
			showHistoryLinks(window,HLS_FAILED);
		}
	}
}

void ChatMessageHandler::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.last-chat-tab-pages").toByteArray();
	QDataStream stream(data);
	stream >> FTabPages;
}

void ChatMessageHandler::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FTabPages;
	Options::setFileValue(data,"messages.last-chat-tab-pages");
}

Q_EXPORT_PLUGIN2(plg_chatmessagehandler, ChatMessageHandler)

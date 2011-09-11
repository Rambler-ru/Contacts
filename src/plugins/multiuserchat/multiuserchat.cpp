#include "multiuserchat.h"

#define SHC_PRESENCE        "/presence"
#define SHC_MESSAGE         "/message"

#define MUC_IQ_TIMEOUT      30000
#define MUC_LIST_TIMEOUT    60000

MultiUserChat::MultiUserChat(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANickName, const QString &APassword, QObject *AParent) : QObject(AParent)
{
	FPresence = NULL;
	FDataForms = NULL;
	FXmppStream = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
	FChatPlugin = AChatPlugin;

	FMainUser = NULL;
	FAutoPresence = false;
	FSHIPresence = -1;
	FSHIMessage = -1;

	FRoomJid = ARoomJid;
	FStreamJid = AStreamJid;
	FNickName = ANickName;
	FPassword = APassword;
	FShow = IPresence::Offline;
	FErrorCode = -1;

	initialize();
}

MultiUserChat::~MultiUserChat()
{
	if (!FUsers.isEmpty())
		closeChat(IPresence::Offline, QString::null);

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPresence);
		FStanzaProcessor->removeStanzaHandle(FSHIMessage);
	}
	emit chatDestroyed();
}

bool MultiUserChat::messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (AOrder==MEO_MULTIUSERCHAT && ADirection==IMessageProcessor::MessageIn && AStreamJid==FStreamJid)
	{
		if (FRoomJid && AMessage.from())
		{
			return processMessage(AMessage.stanza());
		}
	}
	return false;
}

bool MultiUserChat::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	Jid fromJid = AStanza.from();
	Jid toJid = AStanza.to();
	if ( (fromJid && FRoomJid) && (AStreamJid == FStreamJid) )
	{
		AAccept = true;
		if (AHandlerId == FSHIPresence)
		{
			processPresence(AStanza);
		}
		else if (AHandlerId == FSHIMessage)
		{
			processMessage(AStanza);
		}
		return true;
	}
	return false;
}

void MultiUserChat::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (AStanza.id()==FConfigRequestId && FRoomJid==AStanza.from())
	{
		if (AStanza.type() == "result")
		{
			QDomElement formElem = AStanza.firstElement("query",NS_MUC_OWNER).firstChildElement("x");
			while (formElem.namespaceURI() != NS_JABBER_DATA)
				formElem = formElem.nextSiblingElement("x");
			if (!formElem.isNull())
				emit configFormReceived(FDataForms!=NULL ? FDataForms->dataForm(formElem) : IDataForm());
			else
				emit chatNotify(tr("Room configuration is not available."));
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			emit chatError(err.message());
		}
		FConfigRequestId.clear();
	}
	else if (AStanza.id()==FConfigSubmitId && FRoomJid==AStanza.from())
	{
		if (AStanza.type() == "result")
		{
			emit configFormAccepted();
			emit chatNotify(tr("Room configuration accepted."));
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			emit configFormRejected(err.message());
			emit chatError(err.message());
		}
		FConfigSubmitId.clear();
	}
	else if (FAffilListRequests.contains(AStanza.id()) && FRoomJid==AStanza.from())
	{
		QString affiliation = FAffilListRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QList<IMultiUserListItem> listItems;
			QDomElement itemElem = AStanza.firstElement("query",NS_MUC_ADMIN).firstChildElement("item");
			while (!itemElem.isNull())
			{
				if (itemElem.attribute("affiliation") == affiliation)
				{
					IMultiUserListItem listitem;
					listitem.jid = itemElem.attribute("jid");
					listitem.affiliation = itemElem.attribute("affiliation");
					listitem.notes = itemElem.firstChildElement("reason").text();
					listItems.append(listitem);
				}
				itemElem = itemElem.nextSiblingElement("item");
			}
			emit affiliationListReceived(affiliation,listItems);
		}
		else if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			emit chatError(tr("Request for list of %1s is failed: %2").arg(affiliation).arg(err.message()));
		}
	}
	else if (FAffilListSubmits.contains(AStanza.id()) && FRoomJid==AStanza.from())
	{
		QString affiliation = FAffilListSubmits.take(AStanza.id());
		if (AStanza.type() == "error")
		{
			ErrorHandler err(AStanza.element());
			emit chatError(tr("Changes in list of %1s was not accepted: %2").arg(affiliation).arg(err.message()));
		}
		else if (AStanza.type() == "result")
			emit chatNotify(tr("Changes in list of %1s was accepted.").arg(affiliation));
	}
	else if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		emit chatError(err.message());
	}
}

void MultiUserChat::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (AStanzaId == FConfigRequestId)
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		emit chatError(err.message());
		FConfigRequestId.clear();
	}
	else if (AStanzaId == FConfigSubmitId)
	{
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		emit chatError(err.message());
		FConfigRequestId.clear();
	}
	else if (FAffilListRequests.contains(AStanzaId))
	{
		QString affiliation = FAffilListRequests.take(AStanzaId);
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		emit chatError(tr("Request for list of %1s is failed: %2").arg(affiliation).arg(err.message()));
		FAffilListRequests.remove(AStanzaId);
	}
	else if (FAffilListSubmits.contains(AStanzaId))
	{
		QString affiliation = FAffilListSubmits.take(AStanzaId);
		ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
		emit chatError(tr("Changes in list of %1s may not be accepted: %2").arg(affiliation).arg(err.message()));
		FAffilListRequests.remove(AStanzaId);
	}
}

Jid MultiUserChat::streamJid() const
{
	return FStreamJid;
}

Jid MultiUserChat::roomJid() const
{
	return FRoomJid;
}

bool MultiUserChat::isOpen() const
{
	return FMainUser!=NULL;
}

bool MultiUserChat::autoPresence() const
{
	return FAutoPresence;
}

void MultiUserChat::setAutoPresence(bool AAuto)
{
	if (FAutoPresence != AAuto)
	{
		FAutoPresence = AAuto;
		if (FPresence && FAutoPresence)
			setPresence(FPresence->show(),FPresence->status());
	}
}

QList<int> MultiUserChat::statusCodes() const
{
	return FStatusCodes;
}

bool MultiUserChat::isUserPresent(const Jid &AContactJid) const
{
	if (FUsers.contains(AContactJid.resource()) && AContactJid.pBare()==FRoomJid.pBare())
		return true;

	foreach (MultiUser *user, FUsers)
		if (AContactJid == user->data(MUDR_REAL_JID).toString())
			return true;

	return false;
}

IMultiUser *MultiUserChat::mainUser() const
{
	return FMainUser;
}

IMultiUser *MultiUserChat::userByNick(const QString &ANick) const
{
	return FUsers.value(ANick,NULL);
}

QList<IMultiUser *> MultiUserChat::allUsers() const
{
	QList<IMultiUser *> result;
	foreach(MultiUser *user, FUsers)
		result.append(user);
	return result;
}

QString MultiUserChat::nickName() const
{
	return FNickName;
}

void MultiUserChat::setNickName(const QString &ANick)
{
	if (isOpen())
	{
		if (FStanzaProcessor && userByNick(ANick) == NULL)
		{
			Jid userJid(FRoomJid.node(),FRoomJid.domain(),ANick);
			Stanza presence("presence");
			presence.setTo(userJid.eFull());
			FStanzaProcessor->sendStanzaOut(FStreamJid,presence);
		}
	}
	else
	{
		FNickName = ANick;
	}
}

QString MultiUserChat::password() const
{
	return FPassword;
}

void MultiUserChat::setPassword(const QString &APassword)
{
	FPassword = APassword;
}

int MultiUserChat::show() const
{
	return FShow;
}

QString MultiUserChat::status() const
{
	return FStatus;
}

int MultiUserChat::errorCode() const
{
	return FErrorCode;
}

void MultiUserChat::setPresence(int AShow, const QString &AStatus)
{
	if (FStanzaProcessor)
	{
		Jid userJid(FRoomJid.node(),FRoomJid.domain(),FNickName);

		Stanza presence("presence");
		presence.setTo(userJid.eFull());

		QString showText;
		switch (AShow)
		{
		case IPresence::Chat:
			showText = "chat";
			break;
		case IPresence::Away:
			showText = "away";
			break;
		case IPresence::DoNotDisturb:
			showText = "dnd";
			break;
		case IPresence::ExtendedAway:
			showText = "xa";
			break;
		}
		if (AShow == IPresence::Offline || AShow == IPresence::Error || AShow == IPresence::Invisible)
			presence.setType("unavailable");
		else if (!showText.isEmpty())
			presence.addElement("show").appendChild(presence.createTextNode(showText));
		if (!AStatus.isEmpty())
			presence.addElement("status").appendChild(presence.createTextNode(AStatus));

		if (!isOpen() && AShow!=IPresence::Offline && AShow!=IPresence::Error)
		{
			FErrorCode = -1;
			QDomElement xelem = presence.addElement("x",NS_MUC);
			if (!FPassword.isEmpty())
				xelem.appendChild(presence.createElement("password")).appendChild(presence.createTextNode(FPassword));
		}

		FStanzaProcessor->sendStanzaOut(FStreamJid,presence);
	}
}

bool MultiUserChat::sendMessage(const Message &AMessage, const QString &AToNick)
{
	if (isOpen())
	{
		Jid toJid = FRoomJid;
		toJid.setResource(AToNick);

		Message message = AMessage;
		message.setTo(toJid.eFull());
		message.setType(AToNick.isEmpty() ? Message::GroupChat : Message::Chat);

		if (FMessageProcessor == NULL)
		{
			if (FStanzaProcessor && FStanzaProcessor->sendStanzaOut(FStreamJid, message.stanza()))
			{
				emit messageSent(message);
				return true;
			}
		}
		else
		{
			return FMessageProcessor->sendMessage(FStreamJid,message,IMessageProcessor::MessageOut);
		}
	}
	return false;
}

bool MultiUserChat::requestVoice()
{
	if (FStanzaProcessor && isOpen() && FMainUser->data(MUDR_ROLE).toString()==MUC_ROLE_VISITOR)
	{
		Message message;
		message.setTo(FRoomJid.eBare());

		Stanza &mstanza = message.stanza();
		QDomElement formElem = mstanza.addElement("x",NS_JABBER_DATA);
		formElem.setAttribute("type",DATAFORM_TYPE_SUBMIT);

		QDomElement fieldElem =  formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var","FORM_TYPE");
		fieldElem.setAttribute("type",DATAFIELD_TYPE_HIDDEN);
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(MUC_FT_REQUEST));

		fieldElem = formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var",MUC_FV_ROLE);
		fieldElem.setAttribute("type",DATAFIELD_TYPE_TEXTSINGLE);
		fieldElem.setAttribute("label","Requested role");
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(MUC_ROLE_PARTICIPANT));

		return FStanzaProcessor->sendStanzaOut(FStreamJid, mstanza);
	}
	return false;
}

bool MultiUserChat::inviteContact(const Jid &AContactJid, const QString &AReason)
{
	if (FStanzaProcessor && isOpen() && AContactJid.isValid())
	{
		Message message;
		message.setTo(FRoomJid.eBare());

		Stanza &mstanza = message.stanza();
		QDomElement invElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("invite")).toElement();
		invElem.setAttribute("to",AContactJid.eFull());
		if (!AReason.isEmpty())
			invElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(AReason));

		return FStanzaProcessor->sendStanzaOut(FStreamJid, mstanza);
	}
	return false;
}

QString MultiUserChat::subject() const
{
	return FSubject;
}

void MultiUserChat::setSubject(const QString &ASubject)
{
	if (FStanzaProcessor && isOpen())
	{
		Message message;
		message.setTo(FRoomJid.eBare()).setType(Message::GroupChat).setSubject(ASubject);
		FStanzaProcessor->sendStanzaOut(FStreamJid,message.stanza());
	}
}

void MultiUserChat::sendDataFormMessage(const IDataForm &AForm)
{
	if (FStanzaProcessor && FDataForms && isOpen())
	{
		Message message;
		message.setTo(FRoomJid.eBare());
		QDomElement elem = message.stanza().element();
		FDataForms->xmlForm(AForm,elem);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,message.stanza(),0))
			emit dataFormMessageSent(AForm);
	}
}

void MultiUserChat::setRole(const QString &ANick, const QString &ARole, const QString &AReason)
{
	IMultiUser *user = userByNick(ANick);
	if (FStanzaProcessor && user)
	{
		Stanza role("iq");
		role.setTo(FRoomJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement itemElem = role.addElement("query",NS_MUC_ADMIN).appendChild(role.createElement("item")).toElement();
		itemElem.setAttribute("role",ARole);
		itemElem.setAttribute("nick",ANick);
		if (!user->data(MUDR_REAL_JID).toString().isEmpty())
			itemElem.setAttribute("jid",user->data(MUDR_REAL_JID).toString());
		if (!AReason.isEmpty())
			itemElem.appendChild(role.createElement("reason")).appendChild(role.createTextNode(AReason));
		FStanzaProcessor->sendStanzaRequest(this,FStreamJid,role,0);
	}
}

void MultiUserChat::setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason)
{
	IMultiUser *user = userByNick(ANick);
	if (FStanzaProcessor && user)
	{
		Stanza role("iq");
		role.setTo(FRoomJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement itemElem = role.addElement("query",NS_MUC_ADMIN).appendChild(role.createElement("item")).toElement();
		itemElem.setAttribute("affiliation",AAffiliation);
		itemElem.setAttribute("nick",ANick);
		if (!user->data(MUDR_REAL_JID).toString().isEmpty())
			itemElem.setAttribute("jid",user->data(MUDR_REAL_JID).toString());
		if (!AReason.isEmpty())
			itemElem.appendChild(role.createElement("reason")).appendChild(role.createTextNode(AReason));
		FStanzaProcessor->sendStanzaRequest(this,FStreamJid,role,0);
	}
}

bool MultiUserChat::requestAffiliationList(const QString &AAffiliation)
{
	if (FAffilListRequests.values().contains(AAffiliation))
	{
		return true;
	}
	else if (FStanzaProcessor && isOpen() && AAffiliation!=MUC_AFFIL_NONE)
	{
		Stanza iq("iq");
		iq.setTo(FRoomJid.eBare()).setType("get").setId(FStanzaProcessor->newId());
		QDomElement itemElem = iq.addElement("query",NS_MUC_ADMIN).appendChild(iq.createElement("item")).toElement();
		itemElem.setAttribute("affiliation",AAffiliation);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,iq,MUC_LIST_TIMEOUT))
		{
			FAffilListRequests.insert(iq.id(),AAffiliation);
			return true;
		}
		else
		{
			emit chatError(tr("Failed to send request for list of %1s.").arg(AAffiliation));
		}
	}
	return false;
}

bool MultiUserChat::changeAffiliationList(const QList<IMultiUserListItem> &ADeltaList)
{
	if (FStanzaProcessor && isOpen() && !ADeltaList.isEmpty())
	{
		Stanza iq("iq");
		iq.setTo(FRoomJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement query = iq.addElement("query",NS_MUC_ADMIN);
		foreach(IMultiUserListItem listItem, ADeltaList)
		{
			QDomElement itemElem = query.appendChild(iq.createElement("item")).toElement();
			itemElem.setAttribute("affiliation",listItem.affiliation);
			itemElem.setAttribute("jid",listItem.jid);
			if (!listItem.notes.isEmpty())
				itemElem.appendChild(iq.createElement("reason")).appendChild(iq.createTextNode(listItem.notes));
		}
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,iq,MUC_LIST_TIMEOUT))
		{
			emit affiliationListChanged(ADeltaList);
			FAffilListSubmits.insert(iq.id(),ADeltaList.value(0).affiliation);
			return true;
		}
		else
			emit chatError(tr("Failed to send changes in list of %1s").arg(ADeltaList.value(0).affiliation));
	}
	return false;
}

bool MultiUserChat::requestConfigForm()
{
	if (!FConfigRequestId.isEmpty())
	{
		return true;
	}
	else if (FStanzaProcessor && isOpen())
	{
		Stanza iq("iq");
		iq.setTo(FRoomJid.eBare()).setType("get").setId(FStanzaProcessor->newId());
		iq.addElement("query",NS_MUC_OWNER);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,iq,MUC_IQ_TIMEOUT))
		{
			FConfigRequestId = iq.id();
			return true;
		}
	}
	emit chatError(tr("Room configuration request failed."));
	return false;
}

bool MultiUserChat::sendConfigForm(const IDataForm &AForm)
{
	if (!FConfigSubmitId.isEmpty())
	{
		return true;
	}
	else if (FStanzaProcessor && FDataForms)
	{
		Stanza iq("iq");
		iq.setTo(FRoomJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement queryElem = iq.addElement("query",NS_MUC_OWNER).toElement();
		FDataForms->xmlForm(AForm,queryElem);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,iq,MUC_IQ_TIMEOUT))
		{
			FConfigSubmitId = iq.id();
			emit configFormSent(AForm);
			return true;
		}
	}
	emit chatError(tr("Room configuration submit failed."));
	return false;
}

bool MultiUserChat::destroyRoom(const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		Stanza iq("iq");
		iq.setTo(FRoomJid.eBare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement destroyElem = iq.addElement("query",NS_MUC_OWNER).appendChild(iq.createElement("destroy")).toElement();
		destroyElem.setAttribute("jid",FRoomJid.eBare());
		if (!AReason.isEmpty())
			destroyElem.appendChild(iq.createElement("reason")).appendChild(iq.createTextNode(AReason));
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,iq,MUC_IQ_TIMEOUT))
		{
			emit chatNotify(tr("Room destruction request was sent."));
			return true;
		}
	}
	return false;
}

bool MultiUserChat::processMessage(const Stanza &AStanza)
{
	bool hooked = true;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();

	QDomElement statusElem =  AStanza.firstElement("x",NS_MUC_USER).firstChildElement("status");
	while (!statusElem.isNull())
	{
		FStatusCodes.append(statusElem.attribute("code").toInt());
		statusElem = statusElem.nextSiblingElement("status");
	}

	Message message(AStanza);
	if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		emit chatError(err.message());
	}
	else if (message.type()==Message::GroupChat && !message.stanza().firstElement("subject").isNull())
	{
		FSubject = message.subject();
		emit subjectChanged(fromNick, FSubject);
	}
	else if (fromNick.isEmpty())
	{
		if (!AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline").isNull())
		{
			QDomElement declElem = AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline");
			Jid contactJid = declElem.attribute("from");
			QString reason = declElem.firstChildElement("reason").text();
			emit inviteDeclined(contactJid,reason);
		}
		else if (!AStanza.firstElement("x",NS_JABBER_DATA).isNull())
		{
			if (FMessageProcessor == NULL)
				emit dataFormMessageReceived(message);
			else
				hooked = false;
		}
		else
		{
			emit serviceMessageReceived(message);
		}
	}
	else if (!message.body().isEmpty())
	{
		if (FMessageProcessor == NULL)
			emit messageReceived(fromNick,message);
		else
			hooked = false;
	}

	FStatusCodes.clear();
	return hooked;
}

bool MultiUserChat::processPresence(const Stanza &AStanza)
{
	bool accepted = false;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();

	QDomElement xelem = AStanza.firstElement("x",NS_MUC_USER);
	QDomElement itemElem = xelem.firstChildElement("item");

	QDomElement statusElem = xelem.firstChildElement("status");
	while (!statusElem.isNull())
	{
		FStatusCodes.append(statusElem.attribute("code").toInt());
		statusElem = statusElem.nextSiblingElement("status");
	}

	if (AStanza.type().isEmpty())
	{
		if (!fromNick.isEmpty() && !itemElem.isNull())
		{
			int show;
			QString showText = AStanza.firstElement("show").text();
			if (showText.isEmpty())
				show = IPresence::Online;
			else if (showText == "chat")
				show = IPresence::Chat;
			else if (showText == "away")
				show = IPresence::Away;
			else if (showText == "dnd")
				show = IPresence::DoNotDisturb;
			else if (showText == "xa")
				show = IPresence::ExtendedAway;
			else
				show = IPresence::Online;     //Костыль под кривые клиенты и транспорты

			QString status = AStanza.firstElement("status").text();
			Jid realJid = itemElem.attribute("jid");
			QString role = itemElem.attribute("role");
			QString affiliation = itemElem.attribute("affiliation");

			MultiUser *user = FUsers.value(fromNick);
			if (!user)
			{
				user = new MultiUser(FRoomJid,fromNick,this);
				user->setData(MUDR_STREAM_JID,FStreamJid.full());
			}

			user->setData(MUDR_SHOW,show);
			user->setData(MUDR_STATUS,status);
			user->setData(MUDR_REAL_JID,realJid.full());
			user->setData(MUDR_ROLE,role);
			user->setData(MUDR_AFFILIATION,affiliation);

			if (!FUsers.contains(fromNick))
			{
				connect(user->instance(),SIGNAL(dataChanged(int,const QVariant &, const QVariant &)),
					SLOT(onUserDataChanged(int,const QVariant &, const QVariant &)));
				FUsers.insert(fromNick,user);
			}

			if (!isOpen() && (fromNick == FNickName || FStatusCodes.contains(MUC_SC_ROOM_ENTER)))
			{
				FNickName = fromNick;
				FMainUser = user;
				emit chatOpened();
			}

			if (user == FMainUser)
			{
				FShow = show;
				FStatus = status;
				emit presenceChanged(FShow,FStatus);
			}

			emit userPresence(user,show,status);
			accepted = true;
		}
	}
	else if (AStanza.type() == "unavailable")
	{
		MultiUser *user = FUsers.value(fromNick);
		if (user)
		{
			bool applyPresence = true;
			int show = IPresence::Offline;
			QString status = AStanza.firstElement("status").text();
			QString role = itemElem.attribute("role");
			QString affiliation = itemElem.attribute("affiliation");

			if (FStatusCodes.contains(MUC_SC_NICK_CHANGED))       //ChangeNick
			{
				QString newNick = itemElem.attribute("nick");
				FUsers.remove(fromNick);
				FUsers.insert(newNick,user);
				user->setNickName(newNick);
				emit userNickChanged(user,fromNick,newNick);
				if (user == FMainUser)
				{
					FNickName = newNick;
					setPresence(FShow,FStatus);
				}
				applyPresence = false;
			}
			else if (FStatusCodes.contains(MUC_SC_USER_KICKED))   //User kicked
			{
				QString byUser = itemElem.firstChildElement("actor").attribute("jid");
				QString reason = itemElem.firstChildElement("reason").text();
				emit userKicked(fromNick,reason,byUser);
			}
			else if (FStatusCodes.contains(MUC_SC_USER_BANNED))   //User baned
			{
				QString byUser = itemElem.firstChildElement("actor").attribute("jid");
				QString reason = itemElem.firstChildElement("reason").text();
				emit userBanned(fromNick,reason,byUser);
			}
			else if (!xelem.firstChildElement("destroy").isNull())
			{
				QString reason = xelem.firstChildElement("destroy").firstChildElement("reason").text();
				emit roomDestroyed(reason);
			}

			if (applyPresence)
			{
				user->setData(MUDR_SHOW,show);
				user->setData(MUDR_STATUS,status);
				user->setData(MUDR_ROLE,role);
				user->setData(MUDR_AFFILIATION,affiliation);
				emit userPresence(user,show,status);

				if (user != FMainUser)
				{
					FUsers.remove(fromNick);
					delete user;
				}
				else
					closeChat(show,status);
			}
			accepted = true;
		}
	}
	else if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		MultiUser *user = FUsers.value(fromNick);
		if (user)
		{
			user->setData(MUDR_SHOW,IPresence::Error);
			user->setData(MUDR_STATUS,err.message());
			emit userPresence(user,IPresence::Error,err.message());
		}
		emit chatError(err.message());

		if (fromNick != FNickName)
		{
			FUsers.remove(fromNick);
			delete user;
		}
		else
		{
			FErrorCode = err.code();
			closeChat(IPresence::Error,err.message());
		}

		accepted = true;
	}

	FStatusCodes.clear();
	return accepted;
}

void MultiUserChat::initialize()
{
	IPlugin *plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
			FMessageProcessor->insertMessageEditor(MEO_MULTIUSERCHAT,this);
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
		if (FStanzaProcessor)
		{
			IStanzaHandle shandle;
			shandle.handler = this;
			shandle.order = SHO_PI_MULTIUSERCHAT;
			shandle.direction = IStanzaHandle::DirectionIn;
			shandle.streamJid = FStreamJid;
			shandle.conditions.append(SHC_PRESENCE);
			FSHIPresence = FStanzaProcessor->insertStanzaHandle(shandle);

			if (FMessageProcessor == NULL)
			{
				shandle.conditions.clear();
				shandle.order = SHO_MI_MULTIUSERCHAT;
				shandle.conditions.append(SHC_MESSAGE);
				FSHIMessage = FStanzaProcessor->insertStanzaHandle(shandle);
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (presencePlugin)
		{
			FPresence = presencePlugin->findPresence(FStreamJid);
			if (FPresence)
			{
				connect(FPresence->instance(),SIGNAL(changed(int, const QString &, int)),SLOT(onPresenceChanged(int, const QString &, int)));
				connect(FPresence->instance(),SIGNAL(aboutToClose(int, const QString &)),SLOT(onPresenceAboutToClose(int , const QString &)));
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (xmppStreams)
		{
			FXmppStream = xmppStreams->xmppStream(FStreamJid);
			if (FXmppStream)
			{
				connect(FXmppStream->instance(),SIGNAL(jidChanged(const Jid &)),SLOT(onStreamJidChanged(const Jid &)));
				connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
}

void MultiUserChat::closeChat(int AShow, const QString &AStatus)
{
	if (FMainUser)
	{
		FMainUser->setData(MUDR_SHOW,AShow);
		FMainUser->setData(MUDR_STATUS,AStatus);
		emit userPresence(FMainUser,IPresence::Offline,QString::null);
		delete FMainUser;
	}
	FMainUser = NULL;
	FUsers.remove(FNickName);

	foreach(MultiUser *user, FUsers)
	{
		user->setData(MUDR_SHOW,IPresence::Offline);
		user->setData(MUDR_STATUS,QString());
		emit userPresence(user,IPresence::Offline,QString::null);
	}
	qDeleteAll(FUsers);
	FUsers.clear();

	FShow = AShow;
	FStatus = AStatus;
	emit presenceChanged(FShow,FStatus);

	emit chatClosed();
}

void MultiUserChat::onUserDataChanged(int ARole, const QVariant &ABefore, const QVariant &AAfter)
{
	IMultiUser *user = qobject_cast<IMultiUser *>(sender());
	if (user)
		emit userDataChanged(user,ARole,ABefore,AAfter);
}

void MultiUserChat::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
	Q_UNUSED(APriority);
	if (FAutoPresence)
		setPresence(AShow,AStatus);
}

void MultiUserChat::onPresenceAboutToClose(int AShow, const QString &AStatus)
{
	if (FAutoPresence && isOpen())
		setPresence(AShow,AStatus);
}

void MultiUserChat::onStreamClosed()
{
	if (!FUsers.isEmpty())
		closeChat(IPresence::Offline,QString::null);
}

void MultiUserChat::onStreamJidChanged(const Jid &ABefore)
{
	IXmppStream *xmppStream = qobject_cast<IXmppStream *>(sender());
	if (xmppStream)
	{
		FStreamJid = xmppStream->streamJid();
		foreach(MultiUser *user, FUsers)
			user->setData(MUDR_STREAM_JID,FStreamJid.full());
		emit streamJidChanged(ABefore,FStreamJid);
	}
}

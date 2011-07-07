#include "usercontextmenu.h"

UserContextMenu::UserContextMenu(IRostersModel *AModel, IRostersView *AView, IChatWindow *AWindow) : Menu(AWindow->instance())
{
	FRosterIndex = NULL;
	FRostersModel = AModel;
	FRostersView = AView;
	FChatWindow = AWindow;

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
	connect(this,SIGNAL(aboutToHide()),SLOT(onAboutToHide()));
	connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
	connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *,int)),SLOT(onRosterIndexDataChanged(IRosterIndex *,int)));
	connect(FRostersModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRosterIndexRemoved(IRosterIndex *)));
	connect(FChatWindow->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onChatWindowContactJidChanged(const Jid &)));

	onRosterIndexRemoved(FRosterIndex);
}

UserContextMenu::~UserContextMenu()
{

}

bool UserContextMenu::isAcceptedIndex(IRosterIndex *AIndex)
{
	if (AIndex!=NULL && FChatWindow->streamJid()==AIndex->data(RDR_STREAM_JID).toString())
	{
		Jid indexJid = AIndex->data(RDR_PREP_FULL_JID).toString();
		if (FChatWindow->contactJid() == indexJid)
			return true;
		if (indexJid.resource().isEmpty() && (FChatWindow->contactJid() && indexJid))
			return true;
	}
	return false;
}

void UserContextMenu::updateMenu()
{
	if (FRosterIndex)
	{
		Jid jid = FChatWindow->contactJid();
		QString name = FRosterIndex->data(RDR_NAME).toString();
		setTitle(!name.isEmpty() ? QString("<b>%1</b><br>%2").arg(name).arg(jid.bare()): jid.bare());
		menuAction()->setVisible(true);
	}
	else
	{
		setTitle(FChatWindow->contactJid().full());
		menuAction()->setVisible(false);
	}
}

void UserContextMenu::onAboutToShow()
{
	if (FRosterIndex)
		FRostersView->contextMenuForIndex(FRosterIndex,QList<IRosterIndex *>()<<FRosterIndex,RLID_DISPLAY,this);
}

void UserContextMenu::onAboutToHide()
{
	clear();
}

void UserContextMenu::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (FRosterIndex==NULL && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void UserContextMenu::onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == FRosterIndex)
	{
		if (ARole == RDR_PREP_FULL_JID)
		{
			if (isAcceptedIndex(AIndex))
				updateMenu();
			else
				onRosterIndexRemoved(FRosterIndex);
		}
		else if (ARole == RDR_NAME)
		{
			updateMenu();
		}
	}
	else if (FRosterIndex==NULL && ARole==RDR_PREP_FULL_JID && isAcceptedIndex(AIndex))
	{
		FRosterIndex = AIndex;
		updateMenu();
	}
}

void UserContextMenu::onRosterIndexRemoved(IRosterIndex *AIndex)
{
	if (FRosterIndex == AIndex)
	{
		FRosterIndex = FRostersModel->getContactIndexList(FChatWindow->streamJid(),FChatWindow->contactJid()).value(0);
		updateMenu();
	}
}

void UserContextMenu::onChatWindowContactJidChanged(const Jid &ABefour)
{
	Q_UNUSED(ABefour);
	onRosterIndexRemoved(FRosterIndex);
}

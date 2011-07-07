#ifndef SATEWIDGET_H
#define SATEWIDGET_H

#include <QLabel>
#include <QToolButton>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/tabpagenotifypriorities.h>
#include <interfaces/ichatstates.h>
#include <interfaces/imessagewidgets.h>
#include <utils/menu.h>
#include <utils/iconstorage.h>

class StateWidget :
			public QToolButton
{
	Q_OBJECT;
public:
	StateWidget(IChatStates *AChatStates, IChatWindow *AWindow, QWidget *AParent);
	~StateWidget();
protected slots:
	void onStatusActionTriggered(bool);
	void onPermitStatusChanged(const Jid &AContactJid, int AStatus);
	void onUserChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState);
private:
	IChatWindow *FWindow;
	IChatStates *FChatStates;
private:
	Menu *FMenu;
	int FTabNotifyId;
};

#endif // SATEWIDGET_H

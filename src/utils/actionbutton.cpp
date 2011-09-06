#include "actionbutton.h"

#include <QStylePainter>
#include <QStyleOptionButton>
#include "stylestorage.h"

ActionButton::ActionButton(QWidget *AParent) : QPushButton(AParent)
{
	FAction = NULL;
	additionalTextFlag = 0;
	hAlignment = Qt::AlignHCenter;
}

ActionButton::ActionButton(Action *AAction, QWidget *AParent) : QPushButton(AParent)
{
	FAction = NULL;
	additionalTextFlag = 0;
	setAction(AAction);
	hAlignment = Qt::AlignHCenter;
}

Action *ActionButton::action() const
{
	return FAction;
}

void ActionButton::setAction(Action *AAction)
{
	if (FAction != AAction)
	{
		if (FAction)
		{
			disconnect(FAction,0,this,0);
		}

		FAction = AAction;
		onActionChanged();

		if (FAction)
		{
			connect(this,SIGNAL(clicked()),FAction,SLOT(trigger()));
			connect(FAction,SIGNAL(changed()),SLOT(onActionChanged()));
			connect(FAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
			setActionString(AAction->data(Action::DR_UserDefined  + 1).toString());
		}

		emit actionChanged();
	}
}

QString ActionButton::actionString()
{
	return property("actionString").toString();
}

void ActionButton::setActionString(const QString& s)
{
	setProperty("actionString", s);
	StyleStorage::updateStyle(this);
}

void ActionButton::addTextFlag(int flag)
{
	additionalTextFlag = flag;
}

int ActionButton::textHorizontalAlignment() const
{
	return hAlignment;
}

void ActionButton::setTextHorizontalAlignment(int alignment)
{
	hAlignment = alignment;
}

void ActionButton::onActionChanged()
{
	if (FAction)
	{
		setIcon(FAction->icon());
		setText(FAction->text());
		setMenu(FAction->menu());
	}
	else
	{
		setIcon(QIcon());
		setText(QString::null);
		setMenu(NULL);
	}
	emit buttonChanged();
}

void ActionButton::onActionDestroyed(Action *AAction)
{
	if (FAction == AAction)
	{
		setAction(NULL);
	}
}

void ActionButton::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QStyleOptionButton option;
	initStyleOption(&option);
	p.drawControl(QStyle::CE_PushButtonBevel, option);
	option.rect.setBottom(option.rect.bottom() - 1); // dirty hack...
	//p.drawControl(QStyle::CE_PushButtonLabel, option);
	p.drawItemText(option.rect, Qt::AlignVCenter | hAlignment | additionalTextFlag, palette(), isEnabled(), text(), QPalette::ButtonText);
}

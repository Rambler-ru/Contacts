#include "notifytextbrowser.h"

#include <QAbstractTextDocumentLayout>

NotifyTextBrowser::NotifyTextBrowser(QWidget *AParent) : QTextBrowser(AParent)
{
	setFixedHeight(0);
	FMaxHeight = QWIDGETSIZE_MAX;
	connect(this,SIGNAL(textChanged()),SLOT(onTextChanged()),Qt::QueuedConnection);
}

NotifyTextBrowser::~NotifyTextBrowser()
{

}

void NotifyTextBrowser::setMaxHeight(int AMax)
{
	FMaxHeight = AMax;
}

void NotifyTextBrowser::mouseReleaseEvent(QMouseEvent *AEvent)
{
	QWidget::mouseReleaseEvent(AEvent);
}

void NotifyTextBrowser::onTextChanged()
{
	setFixedHeight(qMin(FMaxHeight,qRound(document()->documentLayout()->documentSize().height()) + frameWidth()*2));
	updateGeometry();
}

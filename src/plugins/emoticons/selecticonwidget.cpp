#include "selecticonwidget.h"

#include <QCursor>
#include <QToolTip>
#include <QTextDocument>

SelectIconWidget::SelectIconWidget(IconStorage *AStorage, QWidget *AParent) : QWidget(AParent)
{
	StyleStorage::staticStorage(RSR_STORAGE_STYLESHEETS)->insertAutoStyle(this,STS_EMOTICONS_SELECTICONWIDGET);

	FPressed = NULL;
	FStorage = AStorage;

	FLayout = new QGridLayout(this);
	FLayout->setMargin(2);
	FLayout->setHorizontalSpacing(3);
	FLayout->setVerticalSpacing(3);

	setCursor(Qt::PointingHandCursor);

	createLabels();
}

SelectIconWidget::~SelectIconWidget()
{

}

void SelectIconWidget::createLabels()
{
	QList<QString> keys = FStorage->fileFirstKeys();

	int columns = keys.count()/2 + 1;
	while (columns>1 && columns*columns>keys.count())
		columns--;

	int rows = keys.count() / columns;
	while (keys.count()%rows>0 && (keys.count()/rows)/2<=rows)
		rows--;
	columns = (keys.count()/rows)/2<=rows ? keys.count() / rows : columns;

	int row =0;
	int column = 0;
	foreach(QString key, keys)
	{
		QLabel *label = new QLabel(this);
		label->setObjectName("lblEmoticon");

		label->setMargin(2);
		label->installEventFilter(this);
		label->setAlignment(Qt::AlignCenter);

		QString comment = FStorage->fileOption(key,"comment");
		label->setToolTip(!comment.isEmpty() ? "<b>"+Qt::escape(key)+"</b><br>"+Qt::escape(comment) : "<b>"+Qt::escape(key)+"</b>");
		FStorage->insertAutoIcon(label,key,0,0,"pixmap");

		FKeyByLabel.insert(label,key);
		FLayout->addWidget(label,row,column);

		column = (column+1) % columns;
		row += column==0 ? 1 : 0;
	}
}

bool SelectIconWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	QLabel *label = qobject_cast<QLabel *>(AWatched);
	if (AEvent->type() == QEvent::Enter)
	{
		QToolTip::showText(QCursor::pos(),label->toolTip());
	}
	else if (AEvent->type() == QEvent::MouseButtonPress)
	{
		FPressed = label;
	}
	else if (AEvent->type() == QEvent::MouseButtonRelease)
	{
		if (FPressed == label)
			emit iconSelected(FStorage->subStorage(),FKeyByLabel.value(label));
		FPressed = NULL;
	}
	return QWidget::eventFilter(AWatched,AEvent);
}

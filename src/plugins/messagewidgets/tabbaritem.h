#ifndef TABBARITEM_H
#define TABBARITEM_H

#include <QSize>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <definitions/resources.h>
#include <definitions/stylesheets.h>
#include <interfaces/imessagewidgets.h>
#include <utils/closebutton.h>
#include <utils/iconstorage.h>
#include <utils/stylestorage.h>
#include <utils/customlabel.h>

class TabBarItem :
	public QFrame
{
	Q_OBJECT
	Q_PROPERTY(bool isActive READ isActive WRITE setActive)
	Q_PROPERTY(bool isDraging READ isDraging WRITE setDraging)
	Q_PROPERTY(bool isCloseable READ isCloseable WRITE setCloseable)
	// TODO: add bool properties like "first", "last", "top", "bottom", "left", "right" (maybe some others?)
	// they are needed for more advanced stylesheet stylization
public:
	TabBarItem(QWidget *AParent);
	virtual ~TabBarItem();
	bool isActive() const;
	void setActive(bool AActive);
	bool isDraging() const;
	void setDraging(bool ADraging);
	bool isCloseable() const;
	void setCloseable(bool ACloseable);
	QSize iconSize() const;
	void setIconSize(const QSize &ASize);
	QIcon icon() const;
	void setIcon(const QIcon &AIcon);
	QString iconKey() const;
	void setIconKey(const QString &AIconKey);
	QString text() const;
	void setText(const QString &AText);
	QString toolTip() const;
	void setToolTip(const QString &AToolTip);
	ITabPageNotify notify() const;
	void setNotify(const ITabPageNotify &ANotify);
signals:
	void closeButtonClicked();
protected:
	void showIcon(const QIcon &AIcon);
	void showIconKey(const QString &AIconKey, const QString &AIconStorage);
	void showText(const QString &AText);
	void showToolTip(const QString &AToolTip);
	void showStyleKey(const QString &AStyleKey);
protected:
	virtual void paintEvent(QPaintEvent *AEvent);
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onBlinkTimerTimeout();
private:
	QLabel *FIconLabel;
	CustomLabel *FTextLabel;
	CloseButton *FCloseButton;
private:
	bool FActive;
	bool FDraging;
	QIcon FIcon;
	QSize FIconSize;
	QString FIconKey;
	QString FText;
	QString FToolTip;
private:
	bool FIconHidden;
	QTimer FBlinkTimer;
	ITabPageNotify FNotify;
};

#endif // TABBARITEM_H

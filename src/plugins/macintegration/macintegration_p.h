#ifndef MACINTEGRATION_P_H
#define MACINTEGRATION_P_H

#include <QObject>

class NSImage;
class NSString;
class QImage;

// private class
class MacIntegrationPrivate : public QObject
{
	Q_OBJECT
	friend class MacIntegrationPlugin;
private:
	MacIntegrationPrivate();
public:
	~MacIntegrationPrivate();
	static MacIntegrationPrivate * instance();
	static NSImage * nsImageFromQImage(const QImage & image);
	static QImage qImageFromNSImage(NSImage * image);
	static NSString * nsStringFromQString(const QString & s);
signals:
	void dockClicked();
	void growlNotifyClicked(int);
public:
	void emitClick();
	void emitGrowlNotifyClick(int id);
	// static
	static void setDockBadge(const QString & badgeText);
	static void postGrowlNotify(const QImage & icon, const QString & title, const QString & text, const QString & type, int id);
	static void showGrowlPrefPane();
private:
	static MacIntegrationPrivate * _instance;
};

#endif // MACINTEGRATION_P_H

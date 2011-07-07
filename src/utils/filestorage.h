#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <QList>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include "utilsexport.h"

//Directories and files
#define STORAGE_SHARED_DIR      "shared"
#define STORAGE_DEFFILES_MASK   "*def.xml"

//Common storage options
#define STORAGE_NAME            "name"
#define STORAGE_DESCRIPTION     "description"
#define STORAGE_VERSION         "version"
#define STORAGE_AUTHOR          "author"

class UTILS_EXPORT FileStorage :
			public QObject
{
	Q_OBJECT
	struct StorageObject;
public:
	FileStorage(const QString &AStorage, const QString &ASubStorage = STORAGE_SHARED_DIR, QObject *AParent = NULL);
	virtual ~FileStorage();
	QString storage() const;
	QString subStorage() const;
	void setSubStorage(const QString &ASubStorage);
	QString option(const QString &AOption) const;
	QList<QString> fileKeys() const;
	QList<QString> fileFirstKeys() const;
	int filesCount(const QString AKey) const;
	QString fileName(const QString AKey, int AIndex = 0) const;
	QString fileFullName(const QString AKey, int AIndex = 0) const;
	QString fileMime(const QString AKey, int AIndex = 0) const;
	QString fileOption(const QString AKey, const QString &AOption) const;
	QString fileCacheKey(const QString AKey, int AIndex =0) const;
signals:
	void storageChanged();
public:
	static QList<QString> availStorages();
	static QList<QString> availSubStorages(const QString &AStorage, bool ACheckDefs = true);
	static QList<QString> subStorageDirs(const QString &AStorage, const QString &ASubStorage);
	static QList<QString> resourcesDirs();
	static void setResourcesDirs(const QList<QString> &ADirs);
	static FileStorage *staticStorage(const QString &AStorage);
protected:
	void updateDefinitions();
	void loadDefinitions(const QString &ADefFile, int APrefixIndex);
private:
	QString FStorage;
	QString FSubStorage;
	QList<QString> FPrefixes;
private:
	QList<QString> FKeys;
	QList<StorageObject> FObjects;
	QHash<QString, uint> FKey2Object;
	QHash<QString, QString> FOptions;
private:
	static QList<QString> FMimeTypes;
	static QList<QString> FResourceDirs;
	static QList<FileStorage *> FInstances;
	static QHash<QString, FileStorage *> FStaticStorages;
private:
	static QList<QString> FKeyTags;
	static QList<QString> FFileTags;
	static QList<QString> FObjectTags;
};

#endif // FILESTORAGE_H

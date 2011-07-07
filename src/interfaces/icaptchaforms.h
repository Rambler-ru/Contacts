#ifndef ICAPTCHAFORMS_H
#define ICAPTCHAFORMS_H

#include <QString>
struct IDataForm;

#define CAPTCHAFORMS_UUID   "{b6f038a4-03ff-4d98-a91f-ef6fe7a488a3}"

class ICaptchaForms
{
public:
	virtual QObject *instance() =0;
	virtual bool submitChallenge(const QString &AChallengeId, const IDataForm &ASubmit) =0;
	virtual bool cancelChallenge(const QString &AChallengeId) =0;
protected:
	virtual void challengeReceived(const QString &AChallengeId, const IDataForm &AForm) =0;
	virtual void challengeSubmited(const QString &AChallengeId, const IDataForm &ASubmit) =0;
	virtual void challengeAccepted(const QString &AChallengeId) =0;
	virtual void challengeRejected(const QString &AChallengeId, const QString &AError) =0;
	virtual void challengeCanceled(const QString &AChallengeId) =0;
};

Q_DECLARE_INTERFACE(ICaptchaForms,"Virtus.Plugin.ICaptchaForms/1.0")

#endif

#include "registerdialog.h"

#include <QVBoxLayout>
#include <QTextDocument>

RegisterDialog::RegisterDialog(IRegistration *ARegistration, IDataForms *ADataForms, const Jid &AStremJid,
			       const Jid &AServiceJid, int AOperation, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);

	ui.lneEMail->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lnePassword->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui.lneUserName->setAttribute(Qt::WA_MacShowFocusRect, false);

	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_REGISTERATION,0,0,"windowIcon");

	ui.spgDataForm->setLayout(new QVBoxLayout);
	ui.spgDataForm->layout()->setMargin(0);

	FRegistration = ARegistration;
	FDataForms = ADataForms;
	FStreamJid = AStremJid;
	FServiceJid = AServiceJid;
	FOperation = AOperation;
	FSubmit.serviceJid = AServiceJid;
	FCurrentForm = NULL;

	connect(ARegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
		SLOT(onRegisterFields(const QString &, const IRegisterFields &)));
	connect(ARegistration->instance(),SIGNAL(registerSuccess(const QString &)),
		SLOT(onRegisterSuccess(const QString &)));
	connect(ARegistration->instance(),SIGNAL(registerError(const QString &, const QString &, const QString &)),
		SLOT(onRegisterError(const QString &, const QString &, const QString &)));
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonsClicked(QAbstractButton *)));

	doRegisterOperation();
}

RegisterDialog::~RegisterDialog()
{

}

void RegisterDialog::resetDialog()
{
	setWindowTitle(tr("Registration at %1").arg(FServiceJid.full()));
	if (FCurrentForm)
	{
		ui.spgDataForm->layout()->removeWidget(FCurrentForm->instance());
		FCurrentForm->instance()->deleteLater();
		FCurrentForm = NULL;
	}
	ui.lblInstuctions->setText("");
	ui.lneUserName->setVisible(false);
	ui.lblUserName->setVisible(false);
	ui.lnePassword->setVisible(false);
	ui.lblPassword->setVisible(false);
	ui.lneEMail->setVisible(false);
	ui.lblEmail->setVisible(false);
	ui.stwForm->setCurrentWidget(ui.spgForm);
}

void RegisterDialog::doRegisterOperation()
{
	if (FOperation == IRegistration::Register)
		doRegister();
	else if (FOperation == IRegistration::Unregister)
		doUnregister();
	else if (FOperation == IRegistration::ChangePassword)
		doChangePassword();
	else
		reject();
}

void RegisterDialog::doRegister()
{
	FRequestId = FRegistration->sendRegiterRequest(FStreamJid,FServiceJid);

	resetDialog();
	if (!FRequestId.isEmpty())
		ui.lblInstuctions->setText(tr("Waiting for host response ..."));
	else
		ui.lblInstuctions->setText(tr("Error: Can`t send request to host."));
	ui.dbbButtons->setStandardButtons(QDialogButtonBox::Cancel);
}

void RegisterDialog::doUnregister()
{
	resetDialog();
	ui.lblInstuctions->setText(tr("Do you really want to remove registration from %1?").arg(FServiceJid.full()));
	ui.dbbButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
}

void RegisterDialog::doChangePassword()
{
	resetDialog();
	ui.lblInstuctions->setText(tr("Enter your username and new password."));
	ui.lneUserName->setVisible(true);
	ui.lblUserName->setVisible(true);
	ui.lnePassword->setVisible(true);
	ui.lblPassword->setVisible(true);
	ui.dbbButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
}

void RegisterDialog::onRegisterFields(const QString &AId, const IRegisterFields &AFields)
{
	if (FRequestId == AId)
	{
		resetDialog();

		FSubmit.fieldMask = AFields.fieldMask;
		FSubmit.key = AFields.key;

		if (!AFields.form.type.isEmpty())
		{
			FCurrentForm = FDataForms->formWidget(AFields.form,ui.spgDataForm);
			if (!AFields.form.title.isEmpty())
				setWindowTitle(AFields.form.title);
			ui.spgDataForm->layout()->addWidget(FCurrentForm->instance());
			ui.stwForm->setCurrentWidget(ui.spgDataForm);
		}
		else
		{
			if (!AFields.instructions.isEmpty())
				ui.lblInstuctions->setText(AFields.instructions);

			ui.lneUserName->setText(AFields.username);
			ui.lnePassword->setText(AFields.password);
			ui.lneEMail->setText(AFields.email);

			ui.lneUserName->setVisible((AFields.fieldMask & IRegisterFields::Username) > 0);
			ui.lblUserName->setVisible((AFields.fieldMask & IRegisterFields::Username) > 0);
			ui.lnePassword->setVisible((AFields.fieldMask & IRegisterFields::Password) > 0);
			ui.lblPassword->setVisible((AFields.fieldMask & IRegisterFields::Password) > 0);
			ui.lneEMail->setVisible((AFields.fieldMask & IRegisterFields::Email) > 0);
			ui.lblEmail->setVisible((AFields.fieldMask & IRegisterFields::Email) > 0);

			ui.stwForm->setCurrentWidget(ui.spgForm);
		}
		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	}
}

void RegisterDialog::onRegisterSuccess(const QString &AId)
{
	if (FRequestId == AId)
	{
		resetDialog();
		if (FOperation == IRegistration::Register)
			ui.lblInstuctions->setText(tr("You are successfully registered at %1").arg(FSubmit.serviceJid.full()));
		else if (FOperation == IRegistration::Unregister)
			ui.lblInstuctions->setText(tr("You are successfully unregistered from %1").arg(FSubmit.serviceJid.full()));
		else if (FOperation == IRegistration::ChangePassword)
			ui.lblInstuctions->setText(tr("Password was successfully changed at %1").arg(FSubmit.serviceJid.full()));

		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Close);
	}
}

void RegisterDialog::onRegisterError(const QString &AId, const QString &ACondition, const QString &AMessage)
{
	Q_UNUSED(ACondition);
	if (FRequestId == AId)
	{
		resetDialog();
		ui.lblInstuctions->setText(tr("Requested operation failed: %1").arg(AMessage));
		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Cancel);
	}
}

void RegisterDialog::onDialogButtonsClicked(QAbstractButton *AButton)
{
	QDialogButtonBox::StandardButton button = ui.dbbButtons->standardButton(AButton);
	if (button == QDialogButtonBox::Ok)
	{
		if (FOperation == IRegistration::Register)
		{
			if (!FCurrentForm || FCurrentForm->checkForm(true))
			{
				FSubmit.username = ui.lneUserName->text();
				FSubmit.password = ui.lnePassword->text();
				FSubmit.email = ui.lneEMail->text();
				FSubmit.form = FCurrentForm!=NULL ? FDataForms->dataSubmit(FCurrentForm->userDataForm()) : IDataForm();
				FRequestId = FRegistration->sendSubmit(FStreamJid,FSubmit);
			}
		}
		else if (FOperation == IRegistration::Unregister)
			FRequestId = FRegistration->sendUnregiterRequest(FStreamJid,FServiceJid);
		else if (FOperation == IRegistration::ChangePassword)
			FRequestId = FRegistration->sendChangePasswordRequest(FStreamJid,FServiceJid,ui.lneUserName->text(),ui.lnePassword->text());

		resetDialog();
		if (!FRequestId.isEmpty())
			ui.lblInstuctions->setText(tr("Waiting for host response ..."));
		else
			ui.lblInstuctions->setText(tr("Error: Can`t send request to host."));
		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Cancel);
	}
	else if (button == QDialogButtonBox::Retry)
	{
		doRegisterOperation();
	}
	else if (button == QDialogButtonBox::Cancel)
	{
		setResult(QDialogButtonBox::Cancel);
		close();
	}
	else if (button == QDialogButtonBox::Close)
	{
		setResult(QDialogButtonBox::Close);
		close();
	}
}


#ifndef RIDENTITYFORM_H
#define RIDENTITYFORM_H

#include <QWidget>
#include "ui_formIdentity.h"

class SipClient;
class SipUser;
class SipRegister;
class SipPhoneProxy;

struct Kstate
{
public:
  enum RSipState
  {
    OFFLINE    = 0,
    UNREG      = 1,
    REG        = 2, 
    AUTHREQ    = 3,
    PROC_UNREG = 10,
    PROC_REG   = 11,
    PROC_TRY   = 12,
    LAST = 99
  };
};

class RIdentityForm : public QDialog, public Kstate
{
    Q_OBJECT

public:
    RIdentityForm(QWidget *parent, const char *name, QObject *reg);
    ~RIdentityForm();

private:
    Ui::IdentityDialog ui;

public:
  void setReadOnly( bool mode );
  void updateState( RSipState state );
  void setAutoRegister( bool newAutoRegister ) { ui.autoRegister->setChecked( newAutoRegister ); }
  bool getAutoRegister( void ) { return ui.autoRegister->isChecked(); }
  QString getFullname( void ) const;
  QString getUsername( void ) const;
  QString getHostname( void ) const;
  QString getSipProxy( void ) const;
  QString getSipProxyUsername( void ) const;
  QString getUri( void ) const;
  void setFullname( const QString &newFullname );
  void setUsername( const QString &newUsername );
  void setHostname( const QString &newHostname );
  void setSipProxy( const QString &newSipProxy );
  void setSipProxyUsername( const QString &newSipProxyUsername );
  void setDefault( bool newDefault );

signals:
  void updateForm( void );

public Q_SLOTS:
  virtual void accept();
  virtual void reject();
};



struct SipRegistrationData
{
  // ����� SIP ������� ��� �����������
  // �������� ������ ���� ����� sipUserUri
  QString sipServerUri;
  // ���� SIP �������
  int sipServerPort;
  // ����� �������� �����������
  int registrationExpiresTime;
  // URI �������� ������������
  QString sipUserUri;
  // ����� � ������ ��� �����������
  QString userName;
  QString password;
  // ���� ������������� STUN �������
  bool useStun;
  // ����� STUN ������� ������� ���� � ������� stunserver.domen:3478
  QString stunServerWithPort;
  // �������� qValue
  QString qValue;
  // ���� ���������������
  bool autoRegister;

  SipRegistrationData() : sipServerPort(0), registrationExpiresTime(-1), useStun(false), autoRegister(false) {}
};


class RSipRegistrations : public QObject , public Kstate
{
  Q_OBJECT
public:
  RSipRegistrations(const SipRegistrationData& regData, SipClient *client, SipPhoneProxy *phoneView = 0, QWidget *p = 0, const char *name = 0 );
  ~RSipRegistrations( void );


  void showIdentity( void );
  void unregAllRegistration( void );
  bool getUseStun( void ) { return _useStun; }
  QString getStunSrv( void ) { return _stunServer; }

  // ����� ��� ���������� / ������ �����������
public slots:
  void makeRegister( void );
  void clearRegister( void );
	void trueRegistrationStatusSlot(bool state);

signals:
  // ���� ������ ���������� ������ ������� SipRegister trueRegistrationStatus
  void proxyTrueRegistrationStatus( bool );

private slots:
  void changeRegistration( void );
  void editRegistration( void );
  void update( void );
  void registerStatusUpdated( void );

private:
  void setRegisterState( void );
  void save( void );

  SipClient *_extSipClient;
  RIdentityForm *_pIdentityForm;
  SipPhoneProxy *_extPhoneObject;

  QString _userPrefix;
  SipUser *_pSipUser;
  SipRegister *_sipRegister;

  bool _autoRegister;
  int _expires;
  QWidget *_parentWidget;
  bool _useStun;
  QString _stunServer;
};

#endif // RIDENTITYFORM_H

#ifndef FTPSENDERDAEMON_H
#define FTPSENDERDAEMON_H

#include <QObject>
#include <QFile>

#include "qt4_legacy/qftp.h"
#include "qthreadex.h"
#include "abscallback.h"
#include <QtNetwork/QNetworkSession>
#include <QProgressBar>
#include <QTimer>

#define FTP_RECONNECT_TIMER_INTERVAL 2000
#define FTP_COM_T_OUT_TIMER_INTERVAL 30000

class syncManager;
class QFileInfo;
typedef std::pair<QFileInfo*,int> PAIR_FI_I;

class ftpSenderDaemon : public QObject
{
    Q_OBJECT
public:
    explicit ftpSenderDaemon(Abscallback* cb, int chThreadID);
    QThreadEx* getThread(){return _thread;};
    void sendFile(PAIR_FI_I* fileInfo);
    int getTID() {return _tid;};
    bool startDaemon(QString& err);
    QFtp* getClient(){return _ftp;};
    bool isConnected() { return _connected;};
    void setAsDisconnected() {_connected = false;};

private:
    QThreadEx* _thread;
    syncManager* _syncManager;
    int _tid;

    QFtp *_ftp = nullptr;
    QFile *_file = nullptr;
    QNetworkSession* _ntwkSesstion = nullptr;
    QString _host = "";
    QString _user = "";
    QString _pass = "";
    QString _cdPath = "";
    QFtp::TransferMode _ftpMode = QFtp::Active;
    PAIR_FI_I _currentFileInfo;
    QTimer* _reconnectTimer = nullptr;
    QTimer* _commandTimeoutTimer = nullptr;
    bool _connected = false;

    void initCommandTimer();

private slots:
    void init();
    void ftpCommandFinished(int comId,bool error);
    void onFtpDataTransferProgress(qint64 now,qint64 total);
    void onReconnectTimer();
    void onCommandTimeoutTimer();
    void onFtpcommandStarted(int id);
};

#endif // FTPSENDERDAEMON_H

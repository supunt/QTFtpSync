#include "ftpsenderdaemon.h"
#include "mainwindow.h"
#include "syncmanager.h"
#include "logger/logger.h"

#include <QDebug>

//-----------------------------------------------------------------------------------------------------------------------------------------
ftpSenderDaemon::ftpSenderDaemon(Abscallback* cb, int chThreadID)
{
    _tid = chThreadID;
    _syncManager = (syncManager*)cb;
    _thread =  new QThreadEx(_tid);
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
bool ftpSenderDaemon::startDaemon(QString& err)
{
    err = "";
    init();
    return true;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::init()
{
    _host = MainWindow::getSetting("ftp_host");
    _user = MainWindow::getSetting("ftp_user");
    _pass = MainWindow::getSetting("ftp_pass");
    _ftpMode = MainWindow::getSetting("ftp_pass").toInt()?QFtp::Passive:QFtp::Active;
    _cdPath = MainWindow::getSetting("ftp_cd_path");

    if (_ftp)
    {
        delete _ftp;
        setAsDisconnected();
   }

    _ftp = new QFtp(this);
     connect(_ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(ftpCommandFinished(int,bool)));
     connect(_ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
                        this, SLOT(onFtpDataTransferProgress(qint64,qint64)));
     connect(_ftp, SIGNAL(commandStarted(int)),
                        this, SLOT(onFtpcommandStarted(int)));

      _ftp->setTransferMode(_ftpMode);
      _ftp->connectToHost(_host, 21);

    return;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::onFtpcommandStarted(int id)
{
    initCommandTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::initCommandTimer()
{
    int timeOut= FTP_COM_T_OUT_TIMER_INTERVAL;
    switch (_ftp->currentCommand())
    {
        case QFtp::Login: timeOut = 10000; break;
        case QFtp::ConnectToHost: timeOut = 20000; break;
        case QFtp::Put: timeOut = 10000; break;
        case QFtp::Cd: timeOut = 5000; break;
        default: return; break;
    }

    if (_commandTimeoutTimer)
        delete _commandTimeoutTimer;

    _commandTimeoutTimer = new QTimer(this);
    connect(_commandTimeoutTimer, SIGNAL(timeout()), this,SLOT(onCommandTimeoutTimer()));
    _commandTimeoutTimer->start(timeOut);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::ftpCommandFinished(int comID, bool error)
{
    if (_ftp->currentCommand() == QFtp::ConnectToHost)
    {
        if (error)
        {
            _syncManager->report("Ftp host " + _host + " unreachable.",FTP,ERROR);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            if (!_reconnectTimer)
            {
                _reconnectTimer = new QTimer;
                connect(_reconnectTimer,SIGNAL(timeout()),this,SLOT(onReconnectTimer()));
                _reconnectTimer->start(FTP_RECONNECT_TIMER_INTERVAL);
            }
            return;
        }
        else
        {
            _syncManager->report("Ftp daemon " + QString::number(_tid) +
                                 " connected to FTP service at " + _host
                                 ,FTP,TEXT);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
           if (_reconnectTimer)
           {
               delete _reconnectTimer;
               _reconnectTimer = nullptr;
           }

           if (_user.trimmed() == "")
               _ftp->login();
           else
                _ftp->login(_user,_pass);
        }
    }
    else if (_ftp->currentCommand() == QFtp::Login)
    {
        if (error)
        {
            _syncManager->report("Ftp login failed. Please check login detains in settings window",FTP,ERROR);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            if (!_reconnectTimer)
            {
                _reconnectTimer = new QTimer();
                connect(_reconnectTimer,SIGNAL(timeout()),this,SLOT(onReconnectTimer()));
                _reconnectTimer->start(FTP_RECONNECT_TIMER_INTERVAL);
            }
            return;
        }
        else
        {
            _syncManager->report("Ftp daemon " + QString::number(_tid) +
                                 " logged in to FTP service at " + MainWindow::getSetting("ftp_host")
                                 ,FTP,TEXT);

            _connected = true;

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            if (_reconnectTimer)
            {
                delete _reconnectTimer;
                _reconnectTimer = nullptr;
            }

            if (_cdPath.trimmed() == "")
                _syncManager->onFtpClientConnected();
            else
                _ftp->cd(_cdPath);

            return;
        }
    }
    else if (_ftp->currentCommand() == QFtp::Put)
    {
        if (error)
        {
            _syncManager->report("File '" + _currentFileInfo.first->filePath() +
                                 "' sending failed. ["+ _ftp->errorString() + "].",FTP,ERROR);

            _file->close();
            _file = nullptr;
            _syncManager->onFileUploadStatus(&_currentFileInfo, !error);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            return;
        }
        else
        {
            _syncManager->report("File '" + _currentFileInfo.first->filePath() +
                                 "' sent successfully.",FTP,TEXT);

            _file->close();
            _file = nullptr;
            _syncManager->onFileUploadStatus(&_currentFileInfo, !error);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            return;
        }
    }
    else if (_ftp->currentCommand() == QFtp::Cd)
    {
        if (error)
        {
            _syncManager->report("Change directory failed. Check FTP server and verify the folder " +_cdPath +
                                 "' exists. ["+ _ftp->errorString() + "].",FTP,ERROR);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }
            if (!_reconnectTimer)
            {
                _reconnectTimer = new QTimer();
                connect(_reconnectTimer,SIGNAL(timeout()),this,SLOT(onReconnectTimer()));
                _reconnectTimer->start(FTP_RECONNECT_TIMER_INTERVAL);
            }
            return;
        }
        else
        {
            _syncManager->report("Changed to FTP directory '" + _cdPath +
                                 "' successfully.",FTP,TEXT);

            if (_commandTimeoutTimer)
            {
                delete _commandTimeoutTimer;
                _commandTimeoutTimer = nullptr;
            }

             _syncManager->onFtpClientConnected();
            return;
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::sendFile(PAIR_FI_I* fileInfo)
{
    logger::log("Send request to file '" +fileInfo->first->absoluteFilePath()+ "'");
    _currentFileInfo.first = fileInfo->first;
    _currentFileInfo.second = fileInfo->second;

    _file = new QFile(_currentFileInfo.first->absoluteFilePath(),this);
    _file->open(QFile::ReadOnly);
    if (!_file->isOpen())
    {
        _syncManager->report("File " + _currentFileInfo.first->filePath() +
                             " does not exist for transfer "
                             ,FTP,ERROR);
        return;
    }
    _ftp->put(_file,_currentFileInfo.first->fileName());
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::onFtpDataTransferProgress(qint64 now,qint64 total)
{
    _syncManager->onFileUploadProgress(&_currentFileInfo,now,total);
    initCommandTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::onReconnectTimer()
{
    init();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void ftpSenderDaemon::onCommandTimeoutTimer()
{
    delete _commandTimeoutTimer;
    _commandTimeoutTimer = nullptr;

    _syncManager->onFtpInterrupted();
    if (_ftp->currentCommand() == QFtp::ConnectToHost)
        _syncManager->report("FTP connect timed out. Issuing reconnect.",FTP,ERROR);
    else if (_ftp->currentCommand() == QFtp::Login)
        _syncManager->report("FTP login timed out. Issuing reconnect.",FTP,ERROR);
    else
    {
        _syncManager->report("FTP transfer timed out. Issuing reconnect.",FTP,ERROR);
    }

    init();
}

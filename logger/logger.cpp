#include "logger.h"
#include <qdebug.h>
#include <qdir.h>
#include "mainwindow.h"
#include "syncmanager.h"

logger* logger::_logWriter_ = nullptr;
std::mutex  logger::_logger_mtx_;
QString logger::_logPath = "";
QFile* logger::_fp = nullptr;
QString logger::_default_log_path = QDir::tempPath() +"\\FTPClient";
QTextStream* logger::_logStream = nullptr;
syncManager* logger::_syncMan = nullptr;
//-----------------------------------------------------------------------------------------------------------------------------------------
logger::logger()
{
}
//-----------------------------------------------------------------------------------------------------------------------------------------
 void logger::log(QString log)
 {
    _logger_mtx_.lock();
    logThis(log, "LOG");
    _logger_mtx_.unlock();
 }
 //-----------------------------------------------------------------------------------------------------------------------------------------
  void logger::logWarn(QString log)
  {
     _logger_mtx_.lock();
     logThis(log, "WARNING");
     _logger_mtx_.unlock();
  }
 //-----------------------------------------------------------------------------------------------------------------------------------------
 void logger::logDebug(QString log)
 {
    _logger_mtx_.lock();
    if (_logStream)
    {
       *_logStream << getTimeString() << "|DEBUG|" << log << "\n";
        _logStream->flush();
    }
    _logger_mtx_.unlock();
 }
 //-----------------------------------------------------------------------------------------------------------------------------------------
 void logger::logError(QString log)
 {
    _logger_mtx_.lock();
    logThis(log, "ERROR");
    _logger_mtx_.unlock();
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
 void logger::logThis(QString data, QString typePrefix)
 {
     if (_logStream)
     {
         *_logStream << getTimeString() << "|"+typePrefix+"|" << data << "\n";
         _logStream->flush();
     }
     else
     {
             _syncMan->report("Log stream broken. Contact support",LOGGER,ERROR);
     }
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
 bool logger::init(QString &error, syncManager* syncMan)
 {
     _syncMan = syncMan;
     if (!createDefaultLogPath(error))
         return false;

    _logPath = MainWindow::getSetting("log_path","");
    QString logFile = _logPath + "\\ftpSyncManager.log";

    if (QFile::exists(logFile))
    {
        if (!renameLog())
        {
            error = "Failed to rename existing log '" +_logPath + "'. Contact support";
            return false;
        }
    }

    _fp =new QFile(logFile);
    if (! _fp->open(QIODevice::WriteOnly))
    {
        error = "Failed to initialize log file '" +logFile + "'. Contact Administrator";
        return false;
    }

    _logStream = new QTextStream(_fp);
    if(!_logStream)
    {
        error = "Failed to initialize log stream for '" +logFile + "'. Contact Administrator";
        return false;
    }
    logger::log("Log file initilized.");
    return true;
 }
 //-----------------------------------------------------------------------------------------------------------------------------------------
bool logger::renameLog()
{
    QString newLogName = _logPath + "/ftpSyncManager_"+ getTimeString(true) + ".log";
    QString oldLogName = _logPath + "/ftpSyncManager.log";

    qDebug() << "Backing up old log file as : " << newLogName;
    if (!QFile::rename(oldLogName,newLogName))
    {
         qDebug() << "Backing up old log file failed";
         return false;
    }
    return true;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
QString logger::getTimeString(bool renamer)
{
    time_t timeT;
    struct tm * timeinfo;
   time (&timeT);
   timeinfo = localtime (&timeT);

    char buffer [80];
    if (renamer)
        strftime (buffer,80,"%d%m%y_%H%M%S",timeinfo);
    else
        strftime (buffer,80,"%d%m%y_%H:%M:%S",timeinfo);
    return QString(buffer);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
bool logger::createDefaultLogPath(QString &err)
{
    QDir tempPath = QDir::temp();

    if (!tempPath.exists())
    {
        err = "Create default log path :: ENV['TEMP'] does not exist. Contact administrator.";
        return false;
    }
    if (!tempPath.mkpath("ftpClient"))
    {
        err =  "Create default log path :: '" + QDir::tempPath() +  "/ftpClient' cannot be created. Contact administrator.";
        return false;
    }
    else
    {
         qDebug() << QDir::tempPath() +  " exisits";
    }
    return true;
}

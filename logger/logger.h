#ifndef LOGGER_H
#define LOGGER_H

#include <mutex>
#include <QString>
#include <QTimer>
#include <QFile>
#include <QTextStream>

class syncManager;

class QMainWindow;

class logger
{
public:
     static void log(QString log);
     static void logDebug(QString log);
     static void logError(QString log);
     static void logWarn(QString log);
     static bool init(QString &error, syncManager* syncMan);
private:
    logger();
    logger(const logger&) = delete;
    logger& operator=(logger&) = delete;

    //mutex for _logWriter_, file_desc, etc
    static std::mutex _logger_mtx_;
    static logger* _logWriter_;
    QTimer* _logChunkingTimer;
    static QString _logPath;
    static QFile* _fp;
    static QString _default_log_path;

    static bool renameLog();
    static QString getTimeString(bool renamer = false);
    static bool createDefaultLogPath(QString &err);
    static QTextStream* _logStream;
    static syncManager* _syncMan;
    static void logThis(QString data, QString typePrefix);
};

#endif // LOGGER_H

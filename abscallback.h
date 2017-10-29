#ifndef ABSCALLBACK_H
#define ABSCALLBACK_H

#include "defs.h"
#include "QString"
#include <QTime>

class QFileInfo;
typedef std::pair<QFileInfo*,int> PAIR_FI_I;

class Abscallback
{
public:
    Abscallback();
    virtual void onReportDirScanComplete() = 0;       // This is for discScanner
    virtual void report(QString err, SOURCE source, TWE type) = 0;      // this is for both discScanner and ftpSyncMgr
    virtual void onScanTimerDurationChanged(int newDuration) = 0; // settings window
    virtual void onHKTimerDurationChanged(QTime time) = 0;

    virtual void onFileUploadStatus(PAIR_FI_I* fileinfo, bool status) = 0;
    virtual void onFileUploadProgress(PAIR_FI_I* fileinfo, qint64 now, qint64 total) = 0;
    virtual void onFtpClientConnected() = 0;
    virtual void  onFtpInterrupted() = 0;

};

#endif // ABSCALLBACK_H

#include "qthreadex.h"
#include <QDebug>
#include "logger/logger.h"
//-----------------------------------------------------------------------------------------------------------------------------------------
QThreadEx::QThreadEx(int tid)
{
    _tid = tid;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void QThreadEx::run()
{
    logger::log("FTP Thread " +  QString::number(_tid) + "|" + "FTP daemon " + QString::number(_tid) + " started.");
}

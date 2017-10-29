#include "syncmanager.h"
#include "mainwindow.h"
#include "logger/logger.h"
#include "utils.h"

#include <QDebug>
#include <QtPrintSupport/QPrinter>
#include <QTextDocument>
#include <QProgressBar>
#include <QtNetwork/QNetworkConfiguration>

using namespace std;
//-----------------------------------------------------------------------------------------------------------------------------------------
syncManager::syncManager(QTableWidgetEx* mainViewCtrl,QTableWidgetEx* errorViewCtrl,
                         QTableWidgetEx* startViewCtrl)
{
    _mainViewCtrl = mainViewCtrl;
    _errorViewCtrl = errorViewCtrl;
    _statViewCtrl   = startViewCtrl;

    _directoryScanner = new dirScanner(this);
    QString errStr = "";

    int ftpThrCnt = MainWindow::getSetting("thread_count",QString::number(FTP_DEF_THREAD_COUNT)).toInt();

    _ftpAgents = new ftpSenderDaemon*[ftpThrCnt];

    for (int i = 0; i < ftpThrCnt; ++i)
        _ftpAgents[i] = new ftpSenderDaemon(this,i);


    _cellData = new cellData;
    _cellData->setup(-1,-1,typeid(int).hash_code(),nullptr);
    _syncInterval = MainWindow::getSetting("sync_interval").toInt();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::run()
{
     _manager = new QNetworkConfigurationManager;
    connect(_manager,SIGNAL(configurationChanged(const QNetworkConfiguration &)),
            this,SLOT(onNetworkConfigChange(const QNetworkConfiguration &)));

    createHKAndStatTimer();
    initNetworkSession();
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::createHKAndStatTimer()
{
    _statTimer = new QTimer;
    connect(_statTimer,SIGNAL(timeout()),this,SLOT(onStatTimer()));
    _statTimer->start(STAT_TIMER_INTERVAL*1000);
    report("Stat timer started [Scan interval : " + QString::number(STAT_TIMER_INTERVAL)+
                " seconds]",SYNCMAN,TEXT);

    _houseKeepingTimer = new QTimer;
    QTime hkTime = QTime::fromString(MainWindow::getSetting("qt_spinbox_lineedit"), "h:mm AP");
    QTime curTime = QTime::currentTime();

    int houseKeepingAlarmDuration = curTime.msecsTo(hkTime);

    if (houseKeepingAlarmDuration < 0)
    {
        _isHousekeeping = true;
        report("Application started after housekeeping time.",SYNCMAN,WARNING);
        return;
    }

    connect(_houseKeepingTimer,SIGNAL(timeout()),this,SLOT(onHouseKeepingTimer()));
    _houseKeepingTimer->start(houseKeepingAlarmDuration);
    report("Housekeeping timer started [Triggers at : " + hkTime.toString("h:mm AP")+
                "]",SYNCMAN,TEXT);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::createTransactionTimers()
{
    if (_isHousekeeping)
    {
        report("Transaction timers will not be created as the tool started after Housekeeping time.",
                SYNCMAN,WARNING);
        return;
    }
    _scanLoopTimer = new QTimer;
    connect(_scanLoopTimer,SIGNAL(timeout()),this,SLOT(onDiscScanTimer()));
    _syncInterval = MainWindow::getSetting("sync_interval").toInt();
    _scanLoopTimer->start(_syncInterval*1000);
    report("Disk scan timer started [Scan interval : " + QString::number(_syncInterval) +
                " seconds]",SYNCMAN,TEXT);

    _txTimer= new QTimer;
    connect(_txTimer,SIGNAL(timeout()),this,SLOT(onTransferTimer()));
    _txTimer->start(TX_TIMER_INTERVAL*1000);
    report("Transaction timer started [Scan interval : " + QString::number(TX_TIMER_INTERVAL)+
                " seconds]",SYNCMAN,TEXT);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
 void syncManager::onReportDirScanComplete()
 {
        VEC_FI* newFiles = _directoryScanner->getNewlyAddedFiles();

        report("Directory scan run. "+
               ((newFiles->size() == 0) ? "No":QString::number(newFiles->size())) +
               " new files detected.", SYNCMAN, TEXT);

        int num = 0;
         for (auto file : *newFiles)
         {  
             ((QTableWidgetEx*)_mainViewCtrl)->Insert_Row(file,num);
             _mainTransferQueue.push(new PAIR_FI_I(file,num));
         }
         newFiles->clear();
 }
 //-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onTransferTimer()
{
    // Once main queue starts procesing, files are pulled once FTP is done with one.
    logger::log("On Transfer Timer");
    if (!_mainQProcessing)
       processNextInMasterQueue();
}
  //-----------------------------------------------------------------------------------------------------------------------------------------
   void syncManager::processNextInMasterQueue()
   {
       onStatTimer();
       if (_mainTransferQueue.size() == 0 || !getSyncState())
       {
           _mainQProcessing = false;
            return;
       }

       auto dataPair =_mainTransferQueue.front();
       if (_ftpAgents[0]->getClient() && _ftpAgents[0]->isConnected())
       {
           _mainQProcessing = true;
            _ftpAgents[0]->sendFile(dataPair);
       }
       return;
   }
//-----------------------------------------------------------------------------------------------------------------------------------------
  void syncManager::onFileUploadStatus(PAIR_FI_I* fileinfo, bool status)
  {   
    QProgressBar* wi =(QProgressBar*)_mainViewCtrl->cellWidget(fileinfo->second,3);
    if (!status)
    {
        _mainQProcessing = false;
    }
    else
    {
        _filesTransferred++;
        auto dataPair =_mainTransferQueue.front();
        _mainTransferQueue.pop();
        delete dataPair;

        if (fileinfo->first->size() == 0)
        {
            wi->setMaximum(1);
            wi->setValue(1);
        }
        else
        {
            wi->setMaximum(fileinfo->first->size());
            wi->setValue(fileinfo->first->size());
        }
        processNextInMasterQueue();
    }

    onStatTimer();
  }
  //-----------------------------------------------------------------------------------------------------------------------------------------
  void syncManager::onFileUploadProgress(PAIR_FI_I *fileinfo, qint64 now, qint64 total)
  {
        QProgressBar* wi =(QProgressBar*)_mainViewCtrl->cellWidget(fileinfo->second,3);

        wi->setMaximum(total);
        wi->setValue(now);
  }
 //-----------------------------------------------------------------------------------------------------------------------------------------
 void syncManager::report(QString err, SOURCE source, TWE type)
 {
     switch(type)
     {
        case TEXT:
         logger::log(err);
         break;
     case WARNING:
         logger::logWarn(err);
         break;
     case ERROR:
         logger::logError(err);
         break;
     default:
         logger::log(err);
         break;
     }

     if (_errorViewCtrl->isHidden())
     {
         QSize size = _errorViewCtrl->size();
         _errorViewCtrl->show();
    }
     fe_error* er = nullptr;
     if (_lastError && _lastError->_err == err)
     {
         int val = ++_lastError->_count;
          _cellData->setup(_lastError->_index,3,typeid(int).hash_code(), &val);
        _errorViewCtrl->updateCellValue(_cellData);
        return;
     }
     else
     {
         if (_lastError)
            delete _lastError;

         _lastError = nullptr;
         er = new fe_error;
         er->_source = getSource(source);
         er->_err = err;
         er->_count = 1;
         er->_dt = QDateTime::currentDateTime();
         er->_type = type;

         _lastError = er;
         ((QTableWidgetEx*)_errorViewCtrl)->Insert_Row(er,er->_index);
    }
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
 void syncManager::onDiscScanTimer()
 {
     if (!getSyncState() || !_networkSession || !_networkSession->isOpen())
     {
         if (_isHousekeeping)
         {
            delete _scanLoopTimer;
            _scanLoopTimer = nullptr;
             report("Termination directory scan timer. Entering Housekeeping. Files added after this will not be transferred.",
                    SYNCMAN,WARNING);
         }
         return;
    }

     QString err = "";
    if (!_directoryScanner->OnSyncTimer(err))
        report(err,DIR_SC,ERROR);

    // This is to favour '_scanLoopTimer' timer in the race between housekeeping timer and _scanLoopTimer timer.
    if (_isHousekeeping)
    {
       delete _scanLoopTimer;
       _scanLoopTimer = nullptr;
        report("Termination directory scan timer. Entering Housekeeping. Files added after this will not be transferred.",
               SYNCMAN,WARNING);
    }
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onScanTimerDurationChanged(int newDuration)
{
    if (_syncInterval != newDuration)
    {
        if (_scanLoopTimer)
            delete _scanLoopTimer;

        report("Folder scan interval changed from " + QString::number(_syncInterval) +
               " seconds to " + QString::number(newDuration) + " seconds.", SYNCMAN, WARNING);

        _scanLoopTimer = new QTimer;
        connect(_scanLoopTimer,SIGNAL(timeout()),this,SLOT(onDiscScanTimer()));
        _syncInterval = MainWindow::getSetting("sync_interval").toInt();
        _scanLoopTimer->start(_syncInterval*1000);
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onNetworkSessionError(QNetworkSession::SessionError error)
{
    report("Connection Error : " + _networkSession->errorString() + "(Network name : " +
                _networkSession->configuration().name() + ", error code : " + QString::number(error)
                ,NTWK, ERROR);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onNetworkConnEstablished()
{
    report("Connected to network '" + _networkSession->configuration().name() +
           "' [ Type : " + _networkSession->configuration().bearerTypeName() +
           " ].",SYNCMAN, TEXT);

    QString err = "";
    _ftpAgents[0]->startDaemon(err);
    _isNetworkConnected = true;
    onStatTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::initNetworkSession()
{
    logger::log("Attempt network connection.");
    if (_netConnTimer)
        delete _netConnTimer;

    if (_networkSession)
        delete _networkSession;

    QNetworkConfiguration ntwkConfig;

   logger::log("Looking througn network connections --------.");
   QList<QNetworkConfiguration> ntConf =_manager->allConfigurations(QNetworkConfiguration::Active);
   for (auto cfg : ntConf)
   {
        logger::log("Available active network connection --------.");
        logger::log("\t Name : " + cfg.name());
        logger::log("\t Type : " + cfg.bearerTypeName());

       if (QNetworkConfiguration::BearerUnknown == cfg.bearerType()) // get rid of VMWare adapters
           continue;

       ntwkConfig = cfg;
       break;
   }

    if (ntwkConfig.name() == "")
    {
        report("Cannot connnet to network, starting reconnect timer",SYNCMAN, ERROR);
        _netConnTimer = new QTimer;
        connect(_netConnTimer,SIGNAL(timeout()),this,SLOT(onNetworkReconnectTimer()));
        _netConnTimer->start(2000);
        return;
    }
    _networkSession = new QNetworkSession(ntwkConfig,this);

    // Slots for Network session callbacks

    connect(_networkSession,SIGNAL(opened()),this,SLOT(onNetworkConnEstablished()));
    connect(_networkSession, SIGNAL(error(QNetworkSession::SessionError)),
                this, SLOT(onNetworkSessionError(QNetworkSession::SessionError)));

    _networkSession->open();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onNetworkReconnectTimer()
{
    initNetworkSession();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onNetworkConfigChange(const QNetworkConfiguration &config)
{
    if ( config.bearerTypeName() == "Unknown")
        return;

    QString stateStr = getNetworkStatusString(config.state());

    report("Network connection change detected [ Name : " +
            config.name() + ", Type : " +
           config.bearerTypeName() + ", state : " +
           stateStr + "]", NTWK, (stateStr == "Connected")?SUCCESS:ERROR);

    if (QNetworkConfiguration::Active != config.state())
    {
        _mainQProcessing = false;
        _isNetworkConnected = false;

        if (_ftpAgents[0])
            _ftpAgents[0]->setAsDisconnected();
    }
    onStatTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onFtpInterrupted()
{
    _mainQProcessing = false;
    onStatTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onStatTimer()
{
    if (_statViewCtrl->rowCount() == 0)
       initStatTable();
    else
    {
        QColor qc;
        //-----------------------------------------------------------------------------------------------------------------------------------
        qc.setRgb(255,94,99);
        if (_isNetworkConnected)
            qc.setRgb(111,255,111);

         QString conn = _isNetworkConnected ?"Connected":"Disconnected";
        _cellData->setup(0,1,typeid(QString).hash_code(),
                        &conn, &qc);
        _statViewCtrl->updateCellValue(_cellData);
        //-----------------------------------------------------------------------------------------------------------------------------------
        conn = (_ftpAgents[0] &&  _ftpAgents[0]->isConnected()) ?"Connected":"Disconnected";
        qc.setRgb(255,94,99);
        if (conn == "Connected")
            qc.setRgb(111,255,111);

        _cellData->setup(1,1,typeid(QString).hash_code(),
                         &conn, &qc);

        _statViewCtrl->updateCellValue(_cellData);
        //-----------------------------------------------------------------------------------------------------------------------------------
        int mainQsize =_mainTransferQueue.size();
        _cellData->setup(2,1,typeid(int).hash_code(),
                         &mainQsize);

        _statViewCtrl->updateCellValue(_cellData);
        //-----------------------------------------------------------------------------------------------------------------------------------
        _cellData->setup(3,1,typeid(int).hash_code(),
                         &_filesTransferred);

        _statViewCtrl->updateCellValue(_cellData);
        //-----------------------------------------------------------------------------------------------------------------------------------
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onFtpClientConnected()
{
    createTransactionTimers();
    onStatTimer();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::initStatTable()
{
    QColor qc;
    //---------------------------------------------------------------------------------------------------------
    void* data =  new QString(_isNetworkConnected?"Connected":"Disconnected");

    if (_isNetworkConnected)
        qc.setRgb(111,255,111);
    else
        qc.setRgb(255,94,99);

    statobject* statObj = new statobject("Network", typeid(QString).hash_code(),
                                        data, &qc);

    _statViewCtrl->Insert_Row(statObj);
    //---------------------------------------------------------------------------------------------------------
    bool status = (_ftpAgents[0] &&  _ftpAgents[0]->isConnected());
    data =  new QString(status ?"Connected":"Disconnected");

    if (status)
        qc.setRgb(111,255,111);
    else
        qc.setRgb(255,94,99);

    statObj = new statobject("FTP", typeid(QString).hash_code(),
                                        data, &qc);
    _statViewCtrl->Insert_Row(statObj);
    //---------------------------------------------------------------------------------------------------------
    data = new int(_mainTransferQueue.size());
    statObj = new statobject("Transfer Queue size", typeid(int).hash_code(),
                                         data);
    _statViewCtrl->Insert_Row(statObj);
    //---------------------------------------------------------------------------------------------------------
    data = new int(_filesTransferred);
    statObj = new statobject("Files transferred", typeid(int).hash_code(),
                                         data);

    _statViewCtrl->Insert_Row(statObj);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::onHouseKeepingTimer()
{
    _isHousekeeping = true;
    if (_mainTransferQueue.empty())
    {
        // kill all timers
        delete _scanLoopTimer;   _scanLoopTimer = nullptr;
        delete _netConnTimer;  _netConnTimer = nullptr;
        delete _ftpConnTimer; _ftpConnTimer  = nullptr;
        delete _statTimer; _statTimer = nullptr;
        delete _txTimer; _txTimer  = nullptr;
        delete _houseKeepingTimer; _houseKeepingTimer  = nullptr;

        report("All timers terminated. System entered housekeeping.",SYNCMAN,WARNING);

        // Backup files.
        runDirBackup();
        generateReport();
        return;
     }
    else
    {
         report("Housekeeping on pause. Waiting for queued transfers to complete.",SYNCMAN,WARNING);
        if (_houseKeepingTimer->interval() != STAT_TIMER_INTERVAL)
        {
            delete _houseKeepingTimer;
            _houseKeepingTimer = new QTimer(this);
            connect(_houseKeepingTimer,SIGNAL(timeout()),this,SLOT(onHouseKeepingTimer()));
            _houseKeepingTimer->start(STAT_TIMER_INTERVAL*1000);
            return;
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::runDirBackup()
{
    QDir bkupPath;
    QDate qd = QDate::currentDate();
    QString path = MainWindow::getSetting("bkup_path");
    QString datedDir = "sync_dir_backup"+ qd.toString("_dd_MM_yyyy") + "/";
    bkupPath.setPath(path);
    //--------------------------------------------------------------------------------------------------------------------------------------
    if (!bkupPath.exists(path))
    {
        if (!bkupPath.mkpath(path));
        {
            report("Backup path could not be created. Contact administrator. [Path : " + path + "]."
                   , SYNCMAN, ERROR);
            return;
        }
    }
    //--------------------------------------------------------------------------------------------------------------------------------------
    if (!bkupPath.exists(path+"/"+ datedDir))
    {
        if (!bkupPath.mkdir(datedDir))
        {
            report("Backup directory could not be created. Contact administrator. [Path : " + path + "/"
                    + datedDir + "].",
                   SYNCMAN, ERROR);
            return;
        }
    }
    bkupPath.setPath(path + "/"+ datedDir);
    //--------------------------------------------------------------------------------------------------------------------------------------
    report("Backup path created [Path : " + bkupPath.path() + "].",
           SYNCMAN, TEXT);
    //--------------------------------------------------------------------------------------------------------------------------------------
    report("Backing up files in sync folder paths.", SYNCMAN, TEXT);
    //--------------------------------------------------------------------------------------------------------------------------------------
    int files = _mainViewCtrl->rowCount();
    for (int i=0; i < files; ++i)
    {
       QString pathCellValue = _mainViewCtrl->item(i,5)->text();
       QString fileCellValue = _mainViewCtrl->item(i,4)->text();
        //-----------------------------------------------------------------------------------------------------------------------------------
       if (!bkupPath.exists(bkupPath.path() + "/"+ fileCellValue))
       {
            if (!QFile::copy((pathCellValue + "/"+ fileCellValue), (bkupPath.path() + "/" + fileCellValue)))
            {
               report("Backing up failed for file '" + pathCellValue + "/" +fileCellValue + "'", SYNCMAN, ERROR);
            }
            else
            {
                QFile::remove(pathCellValue+"/"+fileCellValue);
            }
       }
       else
       {
           QFile::remove(pathCellValue+"/"+fileCellValue);
       }
        //-----------------------------------------------------------------------------------------------------------------------------------
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void syncManager::generateReport()
{
        QString fileName = MainWindow::getSetting("bkup_path") + "/report.pdf";
        QPrinter printer(QPrinter::PrinterResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setPaperSize(QPrinter::A4);
        printer.setOutputFileName(fileName);

        QDate date = QDate::currentDate();
        QTextDocument doc;
        QString htmlString = "<h1><font color='#475D87'>Nick Auditore</font></h1>\n";
        htmlString += "<p>Daily FTP report for " + date.toString("ddd, MMMM d yyyy") +"</p>\n";
        htmlString += "<table border-collapse=\"collapse\" border-width=\"thin\" border-sytle=\"solid\" border-color=\"black\">";
        htmlString += "<th>";
        htmlString += "<td bgcolor='#475D87' padding='15px' >File Name</td>";
        htmlString += "<td bgcolor='#475D87' padding='15px'>Originated path</td>";
        htmlString += "<td bgcolor='#475D87' padding='15px'>File Size</td>";
        htmlString += "</th>";

        int files = _mainViewCtrl->rowCount();
        for (int i=0; i < files; ++i)
        {
           htmlString += "<tr>";
           QString name = _mainViewCtrl->item(i,4)->text();
           QString path = _mainViewCtrl->item(i,5)->text();
           QString size = _mainViewCtrl->item(i,1)->text();
           htmlString += "<td>" +name+ "</td>";
           htmlString += "<td>" +path+ "</td>";
           htmlString += "<td>" +size+ "</td>";
           htmlString += "</tr>";
         }
        htmlString += "</table>";
        doc.setHtml(htmlString);
        doc.setPageSize(printer.pageRect().size()); // This is necessary if you want to hide the page number
        doc.print(&printer);
}

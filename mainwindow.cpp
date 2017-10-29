#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "logger/logger.h"
#include <QString>
#include <QDebug>
#include <QLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPalette>
#include <QDesktopWidget>
#include <QMessageBox>

#include <QtPrintSupport/QPrinter>
#include <QTextDocument>

#define MOVE_BETWEEN_SCREENS_OFFSET_PX 200
#define PRIMARY_SCREEN 0

//-----------------------------------------------------------------------------------------------------------------------------------------
QString MainWindow::g_scanDirPath = "";
bool MainWindow::g_mainTblClr = false;
MAP_STR_STR MainWindow::_mapSettings;
//-----------------------------------------------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //----------------------------------------------------------
    QCoreApplication::setOrganizationName("FTPC");
    QCoreApplication::setOrganizationDomain("FTPDomain");
    QCoreApplication::setApplicationName("ftp_sync_mgr");
    //----------------------------------------------------------

    ui->setupUi(this);
    _tableWidget = new QTableWidgetEx(this);
    _errortableWidget = new QTableWidgetEx(this);
    _statTableWidget = new QTableWidgetEx(this);
    _settingsDlg = new settingsDlg(this);
    _settingsDlg->hide();
    SetDefaultDirPath();
    LoadSettings();

    initMainTableHeaders();
    initErrorTableHeaders();
    initStatTableHeaders();
   _errortableWidget->hide();

    QVBoxLayout *layout = new QVBoxLayout;
    QWidget* topLayout = new QWidget;
    topLayout->setLayout(ui->topLayout);

     QWidget* lhWidget = new QWidget;
     QHBoxLayout *lowerHorizontal = new QHBoxLayout;
     lowerHorizontal->addWidget(_errortableWidget);
     lowerHorizontal->addWidget(_statTableWidget);
     lhWidget->setLayout(lowerHorizontal);

     lowerHorizontal->setStretch(0,5);
     lowerHorizontal->setStretch(1,1);

    layout->addWidget(topLayout);
    layout->addWidget(_tableWidget);
    layout->addWidget(lhWidget);

    layout->setStretch(1,2);
    layout->setStretch(2,1);

    QWidget* layedoutWindow = new QWidget;
    layedoutWindow->setLayout(layout);
    setCentralWidget(layedoutWindow);

    ui->txt_path->setText(MainWindow::g_scanDirPath);

     _syncMan = new syncManager(_tableWidget,_errortableWidget,_statTableWidget);
    _settingsDlg->setCallback(_syncMan);

     QPalette pal = ui->btnSync->palette();
     pal.setColor(QPalette::Button, QColor(Qt::red));
     ui->btnSync->setAutoFillBackground(true);
     ui->btnSync->setPalette(pal);
     ui->btnSync->update();

     QString err = "";
     if (!logger::init(err,_syncMan))
     {
         QMessageBox messageBox;
         messageBox.critical(0,"Error",err);
         messageBox.setFixedSize(500,200);
         return;
     }

     connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)),
             this, SLOT(on_ScreenCountChange(int)));
     //------------------------------------------------------------
     _syncMan->run();
 }
//-----------------------------------------------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
   // delete _syncMan;
    delete _settingsDlg;
    SaveSettings();
    delete ui;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::LoadSettings()
{
    _appSetting = new QSettings(QCoreApplication::organizationName(),QCoreApplication::applicationName());

    const QSize defaultSize =  this->size();
    const QPoint defaultloc =  this->pos();
     _appSetting->beginGroup("MainWindow");

     ui->txt_path->setText(_appSetting->value("syncDir", MainWindow::g_scanDirPath).toString());
     MainWindow::g_scanDirPath = ui->txt_path->text();
    _mainTableColumns = _appSetting->value("mainTableCols").toStringList();

    // Screen change after window close
    int screenCnt = QApplication::desktop()->screenCount();
    int lastScreen = _appSetting->value("screen", defaultSize).toInt();
    QSize lastSize = _appSetting->value("size", defaultSize).toSize();
    QPoint lastPoint = _appSetting->value("pos", defaultloc).toPoint();
    QRect screenGeo;

     if (lastScreen > screenCnt - 1)
     {
            screenGeo =  QApplication::desktop()->availableGeometry(PRIMARY_SCREEN);

            if (lastSize.height() > screenGeo.height() - MOVE_BETWEEN_SCREENS_OFFSET_PX)
                lastSize.setHeight(screenGeo.height() - MOVE_BETWEEN_SCREENS_OFFSET_PX);
            if (lastSize.width() > screenGeo.width() - MOVE_BETWEEN_SCREENS_OFFSET_PX)
                lastSize.setWidth(screenGeo.width()- MOVE_BETWEEN_SCREENS_OFFSET_PX);

            lastPoint.setX(MOVE_BETWEEN_SCREENS_OFFSET_PX);
            lastPoint.setY(MOVE_BETWEEN_SCREENS_OFFSET_PX);
     }
     else
        screenGeo =  QApplication::desktop()->availableGeometry(PRIMARY_SCREEN);

     if (screenGeo.height() == lastSize.height() && screenGeo.width() == lastSize.width())
         showFullScreen();
    else
     {
         resize(lastSize);
         move(lastPoint);
     }
     _appSetting->endGroup();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::SaveSettings()
{
    _appSetting->beginGroup("MainWindow");
    _appSetting->setValue("size", size());
    _appSetting->setValue("pos", pos());
    _appSetting->setValue("screen",QApplication::desktop()->screenNumber(this));
    _appSetting->setValue("syncDir",ui->txt_path->text());
    _appSetting->setValue("mainTableCols",_mainTableColumns);
    _appSetting->endGroup();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
 void MainWindow::initMainTableHeaders()
 {
     _mainTableColumns.clear();
     _mainTableColumns << "File Name" << "File Size" << "Last modified time" << "Progress" << "File Name" << "Path";

     _tableWidget->setColumnCount(_mainTableColumns.size());

     _tableWidget->setHorizontalHeaderLabels(_mainTableColumns);
     for (int i = 0; i < _mainTableColumns.size() ; i++)
         _tableWidget->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft);

        _tableWidget->resizeRowsToContents();
        _tableWidget->resizeColumnsToContents();
 }
 //----------------------------------------------------------------------------------------------------------------------------------------
  void MainWindow::initErrorTableHeaders()
  {
      QStringList titles;
      titles << "Reporter" << "Message" <<  "Last reported time" << "Times Repeated" << "Type";
      _errortableWidget->setColumnCount(5);

      _errortableWidget->setHorizontalHeaderLabels(titles);
      for (int i = 0; i < titles.size() ; i++)
         _errortableWidget->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft);

         _errortableWidget->resizeRowsToContents();
         _errortableWidget->resizeColumnsToContents();
         _errortableWidget->setUpdatesEnabled(true);
  }
  //----------------------------------------------------------------------------------------------------------------------------------------
   void MainWindow::initStatTableHeaders()
   {
       QStringList titles;
       titles << "Stat" << "Value" ;
       _statTableWidget->setColumnCount(2);

       _statTableWidget->setHorizontalHeaderLabels(titles);
       for (int i = 0; i < titles.size() ; i++)
          _statTableWidget->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft);

          _statTableWidget->resizeRowsToContents();
          _statTableWidget->resizeColumnsToContents();
          _statTableWidget->setUpdatesEnabled(true);
   }
  //----------------------------------------------------------------------------------------------------------------------------------------
   void MainWindow::onClickSetDirectory()
   {
       QString oldPath = MainWindow::g_scanDirPath;
       _syncMan->setSyncState(false);
       MainWindow::g_scanDirPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                       MainWindow::g_scanDirPath,
                                                       QFileDialog::ShowDirsOnly
                                                       | QFileDialog::DontResolveSymlinks);

       // User cancelled
       if (MainWindow::g_scanDirPath == "")
            MainWindow::g_scanDirPath = oldPath;

        ui->txt_path->setText(MainWindow::g_scanDirPath);

        if (oldPath != MainWindow::g_scanDirPath)
        {
            MainWindow::g_mainTblClr = !MainWindow::g_mainTblClr; // flip color to indicate dir change
            _syncMan->report("Sync directory changed to :" + MainWindow::g_scanDirPath, MAINWND, WARNING);
        }

        _syncMan->setSyncState(true);

   }
  //----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::onChangeSyncState()
{
    QPalette pal = ui->btnSync->palette();
    if (_syncMan->getSyncState())
    {
        _syncMan->report("Haulting sync. If a file is in transfer, it will be completed.",MAINWND,WARNING);
        ui->btnSync->setText("Start sync");
        _syncMan->setSyncState(false);
        pal.setColor(QPalette::Button, QColor(Qt::green));
    }
    else
    {
        _syncMan->report("Resuming sync.",MAINWND,TEXT);
         ui->btnSync->setText("Pause sync");
         _syncMan->setSyncState(true);
          pal.setColor(QPalette::Button, QColor(Qt::red));
    }
    ui->btnSync->setAutoFillBackground(true);
    ui->btnSync->setPalette(pal);
    ui->btnSync->update();
}
//----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::SetDefaultDirPath()
{
    MainWindow::g_scanDirPath = QString(getenv("USERPROFILE")) + "\\Desktop\\files";
}
//----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::onDirPathChange()
{
    QString oldPath = MainWindow::g_scanDirPath;

    MainWindow::g_scanDirPath = ui->txt_path->text();

    if (oldPath != MainWindow::g_scanDirPath)
    {
        _syncMan->report("Sync directory changed to :" + MainWindow::g_scanDirPath, MAINWND, WARNING);
        MainWindow::g_mainTblClr = !MainWindow::g_mainTblClr; // flip
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionSettings_triggered()
{
    _settingsDlg->showWindow();
}
//----------------------------------------------------------------------------------------------------------------------------------------
QString MainWindow::getSetting(QString key ,QString defVal)
{
    MAP_STR_STR::iterator ite = MainWindow::_mapSettings.find(key);
    if (ite == MainWindow::_mapSettings.end())
        return "";
    else
        return ite->second;
}
//----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::updateSetting(QString key, QString value)
{
    MAP_STR_STR::iterator ite = MainWindow::_mapSettings.find(key);
    if (ite == MainWindow::_mapSettings.end())
        return;
    else
    {
        logger::log("Setting change : [Setting : " + ite->first +
                    ", Old Value - "+ ite->second +
                    ", New value - " + value + "].");
        ite->second = value;
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_ScreenCountChange(int newScreenCount)
{
    int currentScreen = QApplication::desktop()->screenNumber(this);

    if (currentScreen < newScreenCount - 1)
    {
        QSize curSize = size();
        bool fullScreen = isFullScreen();

        if (fullScreen)
        {
            showFullScreen();
            return;
        }

        QRect screenGeo =  QApplication::desktop()->availableGeometry(PRIMARY_SCREEN);
        QPoint newPoint(MOVE_BETWEEN_SCREENS_OFFSET_PX,MOVE_BETWEEN_SCREENS_OFFSET_PX);

        if (curSize.height() > screenGeo.height() - MOVE_BETWEEN_SCREENS_OFFSET_PX)
            curSize.setHeight(screenGeo.height() - MOVE_BETWEEN_SCREENS_OFFSET_PX);
        if (curSize.width() > screenGeo.width() - MOVE_BETWEEN_SCREENS_OFFSET_PX)
            curSize.setWidth(screenGeo.width() - MOVE_BETWEEN_SCREENS_OFFSET_PX);

        resize(curSize);
        move(newPoint);
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qtablewidgetex.h"
#include "syncmanager.h"
#include "settingsdlg.h"
#include "QString"
#include <QSettings>

typedef std::map<QString,QString> MAP_STR_STR;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static QString g_scanDirPath;
    static bool g_mainTblClr;
    static MAP_STR_STR _mapSettings;
    static QString getSetting(QString key,QString defVal = "");
    static void updateSetting(QString key, QString value);
private:
    Ui::MainWindow *ui;
    QTableWidgetEx* _tableWidget;
    QTableWidgetEx* _errortableWidget;
    QTableWidgetEx* _statTableWidget;
    syncManager* _syncMan;
    QSettings* _appSetting;
    QStringList _mainTableColumns;

     void initMainTableHeaders();
     void initErrorTableHeaders();
     void initStatTableHeaders();
     void SetDefaultDirPath();
     void LoadSettings();
     void SaveSettings();
     settingsDlg* _settingsDlg;
     QPalette _darkPallet;

     //-------------------------------------------------------------
     void DefaultAppSettings();
    bool LoadAppSettings();
    bool SaveAppSettings();

private slots:
    void onClickSetDirectory();
    void onChangeSyncState();
    void onDirPathChange();
    void on_actionSettings_triggered();
    void on_action_Close_triggered() {close();}
    void on_ScreenCountChange(int newScreenCount);
};

#endif // MAINWINDOW_H

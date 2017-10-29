#include "defs.h"
#include "settingsdlg.h"
#include "ui_settingsdlg.h"
#include "logger/logger.h"

#include <QDebug>
#include <QList>
#include <QDir>
#include <mainwindow.h>
#include <QFileDialog>
#include <QAbstractButton>
#include <QComboBox>

//-----------------------------------------------------------------------------------------------------------------------------------------
settingsDlg::settingsDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDlg)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/Settings.png"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    InitDefaultSettings();
    LoadSettings();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
settingsDlg::~settingsDlg()
{
    saveSettings();
    delete ui;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::LoadSettings()
{
    _generalSettings = new QSettings(QApplication::organizationName(),QApplication::applicationName());
    QList<QLineEdit*> listChildern = this->findChildren<QLineEdit*>();

     _generalSettings->beginGroup("generalSettings");
     QString temp = "";
    for (auto child : listChildern)
    {

        temp =_generalSettings->value(child->objectName()).toString();
        if (temp == "")
        {
            auto ite = _defaultSettings.find(child->objectName());
            if  (ite != _defaultSettings.end())
                temp = ite->second;
        }
        child->setText(temp);
        MainWindow::_mapSettings[child->objectName()] = temp;
        qDebug() << "Loaded Setting : " + child->objectName() + " : " + temp;
    }
    //--------------------------------------------------------------------------------------------------------------------------------------
    QList<QComboBox*> listChildernCmb = this->findChildren<QComboBox*>();
    for (auto child : listChildernCmb)
    {
        temp =_generalSettings->value(child->objectName()).toString();
        if (temp == "")
        {
            auto ite = _defaultSettings.find(child->objectName());
            if  (ite != _defaultSettings.end())
                temp = ite->second;
        }
        child->setCurrentIndex(temp.toInt());
        MainWindow::_mapSettings[child->objectName()] = temp;
        qDebug() << "Loaded Setting : " + child->objectName() + " : " + temp;
    }
    _generalSettings->endGroup();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::saveSettings()
{
    _generalSettings->beginGroup("generalSettings");
    QList<QLineEdit*> listChildern = this->findChildren<QLineEdit*>();

    for (auto child : listChildern)
    {
       _generalSettings->setValue(child->objectName(),child->text());
       MainWindow::updateSetting(child->objectName(),child->text());
    }

    QList<QComboBox*> listChildernCmb = this->findChildren<QComboBox*>();

    for (auto child : listChildernCmb)
    {
       _generalSettings->setValue(child->objectName(),QString::number(child->currentIndex()));
       MainWindow::updateSetting(child->objectName(),QString::number(child->currentIndex()));
    }
    _generalSettings->endGroup();

    doPostChangeSettingValidations();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::doPostChangeSettingValidations()
{
    _syncManager->onScanTimerDurationChanged(ui->sync_interval->text().toInt());
    _syncManager->onHKTimerDurationChanged(ui->house_keeping->time());
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::checkSettings()
{
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::setLogPath()
{
    QString oldPath = ui->log_path->text();
    QString newPath = "";
    newPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                   oldPath,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    // User cancelled
    if (newPath == "")
        newPath = oldPath;

     ui->log_path->setText(newPath);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::setBkupPath()
{
    QString oldPath = ui->bkup_path->text();
    QString newPath = "";
    newPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                   oldPath,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    // User cancelled
    if (newPath == "")
        newPath = oldPath;

     ui->bkup_path->setText(newPath);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::InitDefaultSettings()
{
    _defaultSettings[ui->sync_interval->objectName()] = QString::number(DEFAULT_SCAN_TIMER_INTERVAL);
    _defaultSettings[ui->log_path->objectName()] = QDir::tempPath() +"/FTPClient";
    _defaultSettings[ui->bkup_path->objectName()] = QDir::tempPath() +"/FTPClient/daily_backup";
    _defaultSettings[ui->thread_count->objectName()] = QString::number(FTP_DEF_THREAD_COUNT);
    _defaultSettings[ui->house_keeping->objectName()] = (QTime::fromString("05:00 PM", "h:mm AP")).toString();

    // Add combo values,
    ui->ftp_mode->addItem("Passive", PASSIVE);
    ui->ftp_mode->addItem("Active", ACTIVE);
    _defaultSettings[ui->ftp_mode->objectName()] = 1;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
QString settingsDlg::getDefaultSetting(QString key)
{
    auto ite = _defaultSettings.find(key);
    if (ite == _defaultSettings.end())
        return "";
    else
        return ite->second;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::resetSettings()
{
    _prevSettings.clear();
    //--------------------------------------------------------------------------------------------------------------------------------------
    QList<QLineEdit*> listChildern = this->findChildren<QLineEdit*>();

    for (auto child : listChildern)
    {
        _prevSettings[child->objectName()] = child->text();
        child->setText(getDefaultSetting(child->objectName()));
    }
    //--------------------------------------------------------------------------------------------------------------------------------------
    QList<QComboBox*> listChildernCmb = this->findChildren<QComboBox*>();

    for (auto child : listChildernCmb)
    {
         _prevSettings[child->objectName()] = QString::number(child->currentIndex());
        child->setCurrentIndex(getDefaultSetting(child->objectName()).toInt());
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::onActionBtnClick(QAbstractButton* btn)
{
    if (btn == ui->buttonBox->button(QDialogButtonBox::Reset))
        resetSettings();
    else if (btn == ui->buttonBox->button(QDialogButtonBox::Save))
        saveSettings();
    else if (btn == ui->buttonBox->button(QDialogButtonBox::Cancel))
    {
        revertSettings();
       hide();
    }
}
//--------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::revertSettings()
{
    if (_prevSettings.size() == 0)
        return;
    //--------------------------------------------------------------------------------------------------------------------------------------
    QList<QLineEdit*> listChildern = this->findChildren<QLineEdit*>();

    for (auto child : listChildern)
        child->setText(_prevSettings[child->objectName()]);

    //--------------------------------------------------------------------------------------------------------------------------------------
    QList<QComboBox*> listChildernCmb = this->findChildren<QComboBox*>();

    for (auto child : listChildernCmb)
        child->setCurrentIndex(_prevSettings[child->objectName()].toInt());
}
//--------------------------------------------------------------------------------------------------------------------------------------
void settingsDlg::showWindow()
{
    LoadSettings();
    show();
}
//--------------------------------------------------------------------------------------------------------------------------------------

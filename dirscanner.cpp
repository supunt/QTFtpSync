#include "dirscanner.h"
#include "QFile"
#include "QDebug"
#include "mainwindow.h"

extern QString g_scanDirPath;
//-----------------------------------------------------------------------------------------------------------------------------------------
dirScanner::dirScanner(Abscallback* callback)
{
    _listner = callback;
     _fileTypes <<  "*.pdf";
}
//-----------------------------------------------------------------------------------------------------------------------------------------
bool dirScanner::OnSyncTimer(QString& err)
{
    if (MainWindow::g_scanDirPath == "")
    {
        err = "Scan directory path is empty. Please set the scan directory path.";
         _listner->report("Scan directory path is empty. Please set the scan directory path", DIR_SC, ERROR);
        return false;
    }
    else
    {
        _scanDirectory.setPath( MainWindow::g_scanDirPath);
        if (!_scanDirectory.exists())
        {
            //- windows does not allow existance of dir and file in same name.
             err = "Scan directory path is does not exist. : ";
              _listner->report(err +  MainWindow::g_scanDirPath, DIR_SC, ERROR);
            return false;
        }
        else
            runFileScan();
    }
    return true;
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void dirScanner::runFileScan()
{
    _scanDirectory.setPath( MainWindow::g_scanDirPath);
    _newlyAddedFiles.clear();

    _scanDirectory.setFilter(QDir::Files);
    _scanDirectory.setNameFilters(_fileTypes);

    // Filtered by selected file types, only files-no dirs, sorted by time ascending
    QFileInfoList fil =_scanDirectory.entryInfoList(_fileTypes, QDir::Files, QDir::Time/*|QDir::Reversed*/);

    for (auto fileInfo : fil)
    {
        MAP_QS_FI::iterator ite = _mapMainFileList.find(fileInfo.filePath());
        if (ite == _mapMainFileList.end())
        {
            QFileInfo* qf = new QFileInfo(fileInfo); // copy const
            _mapMainFileList[qf->filePath()] = qf;
            _newlyAddedFiles.push_back(qf);
        }
        else
        {
            //check for MD5 hash TODO
        }

    }
    _listner->onReportDirScanComplete();
}
//-----------------------------------------------------------------------------------------------------------------------------------------

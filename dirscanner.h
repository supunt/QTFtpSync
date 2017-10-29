#ifndef DIRSCANNER_H
#define DIRSCANNER_H

#include "abscallback.h"
#include "QDir"
#include <map>
#include <vector>
#include "qbytearray.h"

typedef std::map<QString,QFileInfo*> MAP_QS_FI;
typedef std::vector<QFileInfo*> VEC_FI;
//-----------------------------------------------------------------------------------------------------------------------------------------
struct QFileInfoEx
{
    QFileInfoEx* fileInfo;
    QByteArray* md5Hash;
};
//-----------------------------------------------------------------------------------------------------------------------------------------
class dirScanner
{
public:
    dirScanner(Abscallback* callback);
    bool OnSyncTimer(QString& err);
    void setDirPath(QString path) {_dirPath = path;};
    QString getDirPath() { return _dirPath ;};
    VEC_FI* const getNewlyAddedFiles() {return &_newlyAddedFiles;};

private:
    Abscallback* _listner       = nullptr;
    QString _dirPath            = "";
    QDir    _scanDirectory;
    MAP_QS_FI _mapMainFileList;
    VEC_FI _newlyAddedFiles;
    QStringList                   _fileTypes;

    void runFileScan();
   // void getMd5Hash(QFileInfoEx* fileInfo) {};
};

#endif // DIRSCANNER_H

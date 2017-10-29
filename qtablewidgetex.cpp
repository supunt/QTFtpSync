#include "qtablewidgetex.h"
#include "QDateTime"
#include "mainwindow.h"
#include "logger/logger.h"

//-----------------------------------------------------------------------------------------------------------------------------------------
QTableWidgetEx::QTableWidgetEx(QWidget *parent):QTableWidget(parent)
{
    _mian_tbl_color[0].setRgb(255,255,255);
    _mian_tbl_color[1].setRgb(255,240,230);
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void QTableWidgetEx::Insert_Row(QFileInfo* qfi, int& rownum)
{
    QColor qc = _mian_tbl_color[(int)MainWindow::g_mainTblClr];
    rownum  = rowCount();
    insertRow(rownum);
    qint64 temp = 0;
    setCellData(rownum,0,&qfi->fileName(),&qc);
    temp = qfi->size();
    QProgressBar* pgb = new QProgressBar();


    pgb->setMinimum(0);
    if (temp == 0)
        pgb->setMaximum(1);
    else
        pgb->setMaximum(temp);

    setCellData(rownum,1,&temp,&qc);
    setCellData(rownum,2,&qfi->created(),&qc);
    setCellData(rownum,3,pgb,&qc);
    setCellData(rownum,4,&qfi->fileName(),&qc);
    setCellData(rownum,5,&qfi->absolutePath(),&qc);

    resizeRowsToContents();
    resizeColumnsToContents();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void QTableWidgetEx::Insert_Row(fe_error* err, int& rownum)
{
    rownum  = 0;
    insertRow(rownum);
    QColor qc;

    switch (err->_type)
    {
        case WARNING: qc.setRgb(255,255,157);break;
        case ERROR: qc.setRgb(255,94,99);break;
        case SUCCESS:qc.setRgb(111,255,111);break;
        default : qc.setRgb(255,255,255);break;
    }
    QString status = getTypeStr(err->_type);

    setCellData(rownum,0,&err->_source,&qc);
    setCellData(rownum,1,&err->_err, &qc);
    setCellData(rownum,2,&err->_dt,&qc);
    setCellData(rownum,3,&err->_count,&qc);
    setCellData(rownum,4,&status,&qc);

    resizeRowsToContents();
    resizeColumnsToContents();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void QTableWidgetEx::Insert_Row(statobject *stat)
{
    if (!stat)
        return;

    int rownum = rowCount();
    insertRow(rownum);

    setCellData(rownum,0,&stat->_statId);
    if (stat->_dataTypeHash == _dt_int_hash)
        setCellData(rownum,1,((int*)stat->_data), &stat->_dataCellColor);
    else if (stat->_dataTypeHash == _dt_double_hash)
        setCellData(rownum,1,((double*)stat->_data), &stat->_dataCellColor);
    else if (stat->_dataTypeHash == _dt_qstring_hash)
        setCellData(rownum,1,((QString*)stat->_data), &stat->_dataCellColor);

    resizeRowsToContents();
    resizeColumnsToContents();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void QTableWidgetEx::updateCellValue(cellData* cellData)
{
    QTableWidgetItem* wi = item(cellData->_row, cellData->_column);
    if (wi)
    {
        if (cellData->_dataTypeHash == _dt_int_hash)
            wi->setText(QString::number(*((int*)cellData->_data)));
        else if (cellData->_dataTypeHash == _dt_double_hash)
            wi->setText(QString::number(*((double*)cellData->_data)));
        else if (cellData->_dataTypeHash == _dt_qstring_hash)
            wi->setText(*((QString*)cellData->_data));
        else if (cellData->_dataTypeHash == _dt_qdatetime_hash)
            wi->setText(((QDateTime*)cellData->_data)->toString());
        else
        {
            logger::logError("Unsupported data type injected.");
        }

         if (cellData->_dataCellColor)
            wi->setBackgroundColor(*cellData->_dataCellColor);
         update();
    }
    resizeRowsToContents();
    resizeColumnsToContents();
}
//-----------------------------------------------------------------------------------------------------------------------------------------
template <typename T>
void QTableWidgetEx::setCellData(int row, int column, T* data, QColor* cellColor)
{
    QTableWidgetItem* twi =  nullptr;
    size_t tid_hash = typeid(*data).hash_code();

     if (tid_hash == _dt_int_hash)
            twi = new QTableWidgetItem(QString::number(*((int*)data)));
     else if (tid_hash == _dt_double_hash)
            twi = new QTableWidgetItem(QString::number(*((double*)data)));
     else if (tid_hash == _dt_qint64_hash)
         twi = new QTableWidgetItem(QString::number(*((qint64*)data)));
    else if (tid_hash == _dt_qdatetime_hash)
            twi = new QTableWidgetItem((*((QDateTime*)data)).toString());
    else if (tid_hash == _dt_qstring_hash )
            twi = new QTableWidgetItem(*((QString*)data));
    else if (tid_hash == _dt_qprogressbar_hash)
    {
         setCellWidget(row,column,(QProgressBar*)data);
         return; // do not continue as this is a widget
    }
     else
     {
         logger::logError("Unsupported data type injected.");
         return;
    }
     if (cellColor)
        twi->setBackgroundColor(*cellColor);

     setItem(row,column,twi);
}

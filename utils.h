#ifndef UTILS_H
#define UTILS_H

#endif // UTILS_H

#pragma once
#include "defs.h"
#include <QString>
#include <QtNetwork/QNetworkConfiguration>

//-----------------------------------------------------------------------------------------------------------------------------------------
QString getNetworkStatusString(QNetworkConfiguration::StateFlags flag)
{
    switch(flag)
    {
        case QNetworkConfiguration::Undefined: return "Disconnected"; break;
        case QNetworkConfiguration::Defined: return "Disconnected"; break;
        case QNetworkConfiguration::Discovered: return "Disconnected"; break;
        case QNetworkConfiguration::Active: return "Connected"; break;
        default : return "unknown"; break;
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------
QString getSource(SOURCE source)
{
    switch (source) {
    case DIR_SC:
        return "File Scanner";
        break;
    case FTP:
        return "FTP";
        break;
    case NTWK:
        return "Network";
        break;
    case MAINWND:
        return "Main Window";
        break;
    case SYNCMAN:
        return "Sync Manager";
        break;
    case LOGGER:
        return "Logger";
        break;
    default:
        return "Other";
        break;
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------------------------------------

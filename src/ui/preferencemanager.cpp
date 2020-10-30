#include "preferencemanager.h"

#include <QStandardPaths>
#include <QDebug>

PreferenceManager& PreferenceManager::getInstance()
{
    static PreferenceManager instance;
    return instance;
}

PreferenceManager::PreferenceManager()
{
    loadPreferences();
}

void PreferenceManager::loadPreferences()
{
    // Load config file
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) ;
    settings = QSharedPointer<QSettings>(new QSettings(configPath + "/config.ini", QSettings::IniFormat));
    qDebug() << "Config location: " << settings->fileName();

    // Set values
    chunkSize = settings->value("chunkSize", 50).toInt();
    wordWrapMode = settings->value("wordWrapMode", 0).toInt();
    writeMode = settings->value("writeMode", 0).toInt();
    byteSizeIndex = settings->value("byteSizeIndex", 1).toInt();
    byteSize = settings->value("byteSize", 1000).toUInt();
}

void PreferenceManager::savePreferences()
{
    settings->setValue("chunkSize", chunkSize);
    settings->setValue("wordWrapMode", wordWrapMode);
    settings->setValue("writeMode", writeMode);
    settings->setValue("byteSizeIndex", byteSizeIndex);
    settings->setValue("byteSize", byteSize);
}

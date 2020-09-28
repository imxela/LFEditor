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
    m_settings = QSharedPointer<QSettings>(new QSettings(configPath + "/config.ini", QSettings::IniFormat));
    qDebug() << "Config location: " << m_settings->fileName();

    // Set values
    blockSize = m_settings->value("blockSize", 50).toInt();
    wordWrapMode = m_settings->value("wordWrapMode", 0).toInt();
    writeMode = m_settings->value("writeMode", 0).toInt();
    byteSizeIndex = m_settings->value("byteSizeIndex", 1).toInt();
    byteSize = m_settings->value("byteSize", 1000).toUInt();
}

void PreferenceManager::savePreferences()
{
    m_settings->setValue("blockSize", blockSize);
    m_settings->setValue("wordWrapMode", wordWrapMode);
    m_settings->setValue("writeMode", writeMode);
    m_settings->setValue("byteSizeIndex", byteSizeIndex);
    m_settings->setValue("byteSize", byteSize);
}

#include "preferencemanager.h"

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
    m_settings = QSharedPointer<QSettings>(new QSettings(QString("configs/config.ini"), QSettings::IniFormat));

    // Set values
    blockSize = m_settings->value("blockSize", 50).toInt();
    wordWrapMode = m_settings->value("wordWrapMode", 0).toInt();
    writeMode = m_settings->value("writeMode", 0).toInt();
}

void PreferenceManager::savePreferences()
{
    m_settings->setValue("blockSize", blockSize);
    m_settings->setValue("wordWrapMode", wordWrapMode);
    m_settings->setValue("writeMode", writeMode);
}

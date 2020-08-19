#ifndef PREFERENCEMANAGER_H
#define PREFERENCEMANAGER_H

#include <QVariant>
#include <QSettings>
#include <QSharedPointer>

class PreferenceManager
{
public:
    static PreferenceManager& getInstance();

    void loadPreferences();
    void savePreferences();

    int blockSize;
    int wordWrapMode;

private:
    PreferenceManager();

    QSharedPointer<QSettings> m_settings;

};

#endif // PREFERENCEMANAGER_H

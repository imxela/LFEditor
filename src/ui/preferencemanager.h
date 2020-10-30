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

    int chunkSize;
    int wordWrapMode;
    int writeMode;
    int byteSizeIndex;
    uint byteSize;

private:
    PreferenceManager();

    QSharedPointer<QSettings> settings;

};

#endif // PREFERENCEMANAGER_H

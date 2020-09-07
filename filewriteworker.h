#ifndef FILEWRITEWORKER_H
#define FILEWRITEWORKER_H

#include <QObject>
#include <QFile>
#include <QSharedPointer>
#include <QByteArray>

class FileWriteWorker : public QObject
{
    Q_OBJECT

public:
    FileWriteWorker();
    virtual ~FileWriteWorker();
    
    void sendError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);

public slots:
    void writeFile(QFile* file, qint64 from, QByteArray bytes, quint64 blockSize, bool simpleWrite);

signals:
    void finished();
    void error(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);
    void readyForDelete();

};

#endif // FILEWRITEWORKER_H

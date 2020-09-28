#ifndef FILEWRITEWORKER_H
#define FILEWRITEWORKER_H

#include "workerbase.h"

#include <QObject>
#include <QFile>
#include <QSharedPointer>
#include <QByteArray>

class FileWriteWorker : public WorkerBase
{
    Q_OBJECT

public:
    FileWriteWorker(QObject* parent = nullptr);
    virtual ~FileWriteWorker();
    
private:
    void readFromWriteTo(qint64 readFrom, qint64 writeTo, qint64 count, QByteArray* source, QFile* target);

public slots:
    void writeFile(QFile* file, qint64 from, QByteArray bytes, qint64 blockSize, bool simpleWrite);

};

#endif // FILEWRITEWORKER_H

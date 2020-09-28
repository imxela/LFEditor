#ifndef FILEREADWORKER_H
#define FILEREADWORKER_H

#include "workerbase.h"

#include <QObject>
#include <QByteArray>
#include <QFile>

class FileReadWorker : public WorkerBase
{
    Q_OBJECT

public:
    FileReadWorker(QObject* parent = nullptr);
    virtual ~FileReadWorker();
    
public slots:
    void readFile(QFile* file, qint64 from, qint64 to);

signals:
    void result(qint32 bytesRead, QByteArray* bytes);

private:
    QByteArray* bytes;

};

#endif // FILEREADWORKER_H

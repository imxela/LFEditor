#ifndef FILEREADWORKER_H
#define FILEREADWORKER_H

#include <QObject>
#include <QByteArray>
#include <QFile>

class FileReadWorker : public QObject
{
    Q_OBJECT

public:
    FileReadWorker();
    virtual ~FileReadWorker();

public slots:
    void readFile(QFile* file, qint64 from, qint64 to);

signals:
    void finished(qint32 bytesRead, QByteArray* bytes);
    void readyForDelete();

private:
    QByteArray* m_bytes;

};

#endif // FILEREADWORKER_H

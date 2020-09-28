#include "filereadworker.h"

#include <QDebug>
#include <QByteArray>
#include <QThread>

FileReadWorker::FileReadWorker(QObject* parent) :
    WorkerBase(parent)
{
}

FileReadWorker::~FileReadWorker()
{
    
}

void FileReadWorker::readFile(QFile* file, qint64 from, qint64 to)
{
    qint64 bytesToRead = to - from;
    QScopedPointer<char> fileContent(new char[bytesToRead]);
    m_bytes = new QByteArray(bytesToRead, ' '); // Fill with blanks for now

    qDebug() << "Reading " << QString::number(bytesToRead) << " bytes";

    file->seek(from);
    qint64 bytesRead = file->read(fileContent.data(), bytesToRead);
    if (bytesRead < 0)
    {
        QString desc("Failed to read file: '%1'.");
        desc = desc.arg(file->fileName());

        reportError("File read error", desc, file->errorString(), file->error());
        return;
    }
    
    m_bytes->resize(bytesRead); // We can resize the array here, no reason to have unused elements
    
    for (int i = 0; i < m_bytes->count(); i++)
    {
        m_bytes->data()[i] = fileContent.data()[i];
    }

    emit result(bytesRead, m_bytes);
    finishExecution();
}

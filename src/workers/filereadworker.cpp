#include "filereadworker.h"

#include <QDebug>
#include <QByteArray>
#include <QThread>

FileReadWorker::FileReadWorker()
{

}

FileReadWorker::~FileReadWorker()
{
    
}

void FileReadWorker::sendError(const QString &title, const QString &description, const QString& errorString, qint64 errorCode)
{
    emit error(title, description, errorString, errorCode);
    thread()->exit(EXIT_FAILURE);
}

void FileReadWorker::readFile(QFile* file, qint64 from, qint64 to)
{
    qint64 bytesToRead = to - from;
    QScopedPointer<char> fileContent(new char[bytesToRead]);
    m_bytes = new QByteArray(bytesToRead, ' '); // Fill with null-terminator for now

    qDebug() << "Reading " << QString::number(bytesToRead) << " bytes";

    file->seek(from);
    qint64 bytesRead = file->read(fileContent.data(), bytesToRead);
    if (bytesRead < 0)
    {
        QString desc("Failed to read file: '%1'.");
        desc = desc.arg(file->fileName());

        sendError("File read error", desc, file->errorString(), file->error());
        return;
    }
    
    m_bytes->resize(bytesRead); // We can resize the array here, no reason to have unused elements
    
    for (int i = 0; i < m_bytes->count(); i++)
    {
        m_bytes->data()[i] = fileContent.data()[i];
    }

    emit finished(bytesRead, m_bytes);
    emit readyForDelete();
    
    thread()->quit();
}

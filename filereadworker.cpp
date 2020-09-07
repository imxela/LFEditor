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
        QString errorDescription("Failed to read file: '%1'.");
        errorDescription.arg(file->fileName());

        sendError("File read error", errorDescription, file->errorString(), file->error());
        return;
    }

    for (int i = 0; i < m_bytes->count(); i++)
    {
        m_bytes->data()[i] = fileContent.data()[i];
    }

    // Todo: Move the resize to before the for-loop, I'm looping over thousands
    //       of elements unnecessairly.
    m_bytes->resize(bytesRead); // We can resize the array here, no reason to have unused elements

    emit finished(bytesRead, m_bytes);
    emit readyForDelete();

    thread()->quit();
    if (!thread()->wait(5000))
    {
        // Failed to quit thread, possibly deadlocked, terminate instead
        thread()->terminate();
    }
    thread()->wait(); // Wait for deleteLater();
}

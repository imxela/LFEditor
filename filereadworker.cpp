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

void FileReadWorker::readFile(QFile* file, qint64 from, qint64 to)
{

    qint64 bytesToRead = to - from;
    QScopedPointer<char> fileContent(new char[bytesToRead]);
    m_bytes = new QByteArray(bytesToRead, '\0'); // Fill with null-terminator for now

    qDebug() << "Reading " << QString::number(bytesToRead) << " bytes";

    file->seek(from);
    qint64 bytesRead = file->read(fileContent.data(), bytesToRead);
    if (bytesRead < 0)
    {
        qDebug() << "Failed to read file '" << file->fileName() << "': " << file->error();
        // QMessageBox::critical(this, "File Error", QString("Failed to read file '%l'.\nReason: %l").arg(fileName, file.error()));
    }

    for (int i = 0; i < m_bytes->size(); i++)
    {
        m_bytes->data()[i] = fileContent.data()[i];
    }

    m_bytes->resize(bytesRead); // We can resize the array here, no reason to have unused elements

    // Emits < 0 on fail
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

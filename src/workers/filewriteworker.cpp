#include "filewriteworker.h"

#include <QDebug>
#include <QThread>

FileWriteWorker::FileWriteWorker()
{

}

FileWriteWorker::~FileWriteWorker()
{
    qDebug() << "FileWriteWorker destroyed!";
}

void FileWriteWorker::sendError(const QString &title, const QString &description, const QString &errorString, qint64 errorCode)
{
    emit error(title, description, errorString, errorCode);
}

void FileWriteWorker::readFromWriteTo(qint64 readFrom, qint64 writeTo, qint64 count, QByteArray* source, QFile* target)
{
    for (qint64 i = 0; i < count; i++)
    {   
        if (!target->seek(readFrom))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(target->fileName());
        
            sendError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        char buffer = '\0';
        qint64 readCount = target->read(&buffer, 1);
        if (readCount < 1)
        {
            QString desc("Failed to read file '%1'.\nExpected %2 characters, but only %3 were read.");
            desc = desc.arg(target->fileName(), QString::number(1), QString::number(readCount));
        
            sendError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        if (!target->seek(writeTo))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(target->fileName());
        
            sendError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        qint64 writeCount = target->write(&buffer, 1);
        if (writeCount < 1)
        {
            QString desc("Failed to write file '%1'.\nExpected %2 characters, but only %3 were written.");
            desc = desc.arg(target->fileName(), QString::number(1), QString::number(writeCount));
        
            sendError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        qDebug() << "Wrote '" << buffer << "' from " << readFrom << " to " << writeTo;
        
        // Increment to continue to the next character
        readFrom++;
        writeTo++;
    }
}

void FileWriteWorker::writeFile(QFile* file, qint64 from, QByteArray bytes, quint64 blockSize, bool simpleWrite)
{
    // If simpleWrite is true, we do not need to delete removed characters from the file
    if (simpleWrite)
    {
        /* Simple Write */
        qDebug() << "Starting simple write!";
        
        if (!file->seek(from))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(file->fileName());
        
            sendError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        // Write new data to file block
        if (file->write(bytes) < bytes.size())
        {
            QString desc("Failed to write to file '%1'.");
            desc = desc.arg(file->fileName());
        
            sendError("File Error", desc, file->errorString(), file->error());
            return;
        }
    }
    else
    {   
        /* Complex Write */
        
        qDebug() << "Starting complex write";
        qDebug() << "bytes.size() is " << QString::number(bytes.size());
        qDebug() << "blockSize is " << QString::number(blockSize);
        qDebug() << "file->size() is " << QString::number(file->size());
        
        // Step 1: Write the new block data to the file
        
        if (!file->seek(from))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(file->fileName());
        
            sendError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        if (file->write(bytes) < bytes.size())
        {
            QString desc("Failed to write to file '%1'.");
            desc = desc.arg(file->fileName());
        
            sendError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        // Step 2: If needed, move all data after the end of the old block to the end of the new block (this overwrites the deleted data)
        //         The end of the new block is located at 'bytes.size()', and the end of the old block at 'blockSize'
        
        if (blockSize < static_cast<quint64>(file->size())) 
        {
            readFromWriteTo(from + blockSize, from + bytes.size(), blockSize - bytes.size(), &bytes, file);
        }
        else
        {
            qDebug() << "blockSize size is larger or same size as file, no moving of data required. ";
        }
        
        // Step 3: Resize the file to remove the dangling data at the end of the file since the real data has been copied & moved upwards in the file
        qint64 oldFileSize = file->size();
        file->resize(file->size() - (blockSize - bytes.size()));
        qDebug() << "Resized file from " << oldFileSize << " bytes to " << file->size() << "bytes.";
    }

    emit finished();
    emit readyForDelete();
}

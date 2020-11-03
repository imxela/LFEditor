#include "filewriteworker.h"

#include <QDebug>
#include <QThread>

FileWriteWorker::FileWriteWorker(QObject* parent) :
    WorkerBase(parent)
{

}

FileWriteWorker::~FileWriteWorker()
{
    qDebug() << "FileWriteWorker destroyed!";
}

void FileWriteWorker::readFromWriteTo(qint64 readFrom, qint64 writeTo, qint64 count/*, QByteArray* source*/, QFile* target)
{
    for (qint64 i = 0; i < count; i++)
    {   
        if (!target->seek(readFrom))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(target->fileName());
        
            reportError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        char buffer = '\0';
        qint64 readCount = target->read(&buffer, 1);
        if (readCount < 1)
        {
            QString desc("Failed to read file '%1'.\nExpected %2 characters, but only %3 were read.");
            desc = desc.arg(target->fileName(), QString::number(1), QString::number(readCount));
        
            reportError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        if (!target->seek(writeTo))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(target->fileName());
        
            reportError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        qint64 writeCount = target->write(&buffer, 1);
        if (writeCount < 1)
        {
            QString desc("Failed to write file '%1'.\nExpected %2 characters, but only %3 were written.");
            desc = desc.arg(target->fileName(), QString::number(1), QString::number(writeCount));
        
            reportError("File Error", desc, target->errorString(), target->error());
            return; 
        }
        
        qDebug() << "Wrote '" << buffer << "' from " << readFrom << " to " << writeTo;
        
        // Increment to continue to the next character
        readFrom++;
        writeTo++;
    }
}

void FileWriteWorker::writeFile(QFile* file, qint64 from, QByteArray bytes, qint64 chunkSize, bool simpleWrite)
{
    if (!file->isOpen())
        file->open(QIODevice::ReadWrite);
    
    // If simpleWrite is true, we do not need to delete removed characters from the file
    if (simpleWrite)
    {
        /* Simple Write */
        qDebug() << "Starting simple write!";
        qDebug() << file->fileName();
        
        if (!file->seek(from))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(file->fileName());
        
            reportError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        // Write new data to file chunk
        if (file->write(bytes) < bytes.size())
        {
            QString desc("Failed to write to file '%1'.");
            desc = desc.arg(file->fileName());
        
            reportError("File Error", desc, file->errorString(), file->error());
            return;
        }
    }
    else
    {   
        /* Complex Write */
        qDebug() << "Starting complex write";
        qDebug() << "bytes.size() is " << QString::number(bytes.size());
        qDebug() << "chunkSize is " << QString::number(chunkSize);
        qDebug() << "file->size() is " << QString::number(file->size());
        
        // Step 1: Write the new chunk data to the file
        
        if (!file->seek(from))
        {
            QString desc("Failed to seek file '%1'.");
            desc = desc.arg(file->fileName());
        
            reportError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        if (file->write(bytes) < bytes.size())
        {
            QString desc("Failed to write to file '%1'.");
            desc = desc.arg(file->fileName());
        
            reportError("File Error", desc, file->errorString(), file->error());
            return;
        }
        
        // Step 2: If needed, move all data after the end of the old chunk to the end of the new chunk (this overwrites the deleted data)
        //         The end of the new chunk is located at 'bytes.size()', and the end of the old chunk at 'chunkSize'
        
        if (chunkSize < file->size()) 
        {
            readFromWriteTo(from + chunkSize, from + bytes.size(), chunkSize - bytes.size()/*, &bytes*/, file);
        }
        else
        {
            qDebug() << "chunkSize size is larger or same size as file, no moving of data required. ";
        }
        
        // Step 3: Resize the file to remove the dangling data at the end of the file since the real data has been copied & moved upwards in the file
        qint64 oldFileSize = file->size();
        file->resize(file->size() - (chunkSize - bytes.size()));
        qDebug() << "Resized file from " << oldFileSize << " bytes to " << file->size() << "bytes.";
    }

    finishExecution();
}

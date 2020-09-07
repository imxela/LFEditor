#include "filewriteworker.h"

#include <QDebug>
#include <QThread>

FileWriteWorker::FileWriteWorker()
{

}

FileWriteWorker::~FileWriteWorker()
{

}

void FileWriteWorker::sendError(const QString &title, const QString &description, const QString &errorString, qint64 errorCode)
{
    emit error(title, description, errorString, errorCode);
}

void FileWriteWorker::writeFile(QFile* file, qint64 from, QByteArray bytes, quint64 blockSize, bool simpleWrite)
{ 
    qDebug() << "simpleWrite: " << simpleWrite;
    
    if (simpleWrite == false)
    {
        sendError("Error", "Complex writing has not yet been implemented!", "N/A", 0);
        return;
    }
    
    // If simpleWrite is true, we do not need to delete removed characters from the file
    if (simpleWrite)
    {
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
        // Match the bytes array to the size of a block and fill the new elements with null-terminators
        // The null-terminators are used to determine what data has been deleted by the user
        qint64 oldSize = bytes.size();
        bytes.resize(blockSize);
        for (int i = oldSize; i < bytes.size(); i++)
        {
            bytes[i] = '\0';
        }

        // Go to start of block
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

        qint64 endOfEditIndex = -1;
        qint64 emptyCharacterCount = -1;

        // Find the end of the edited text (first occurrence of a null-terminator)
        // Start at 'from', the end can't be before the unedited content
        for (qint64 i = from; i < file->size(); i++)
        {  
            char buffer = '\0';
            if (!file->seek(i))
            {
                QString desc("Failed to seek file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        
            if (file->write(&buffer, 1) < 1)
            {
                QString desc("Failed to write to file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }

            if (buffer == '\0')
            {
                // End of edit has been found
                endOfEditIndex = i; // The index of the first null-terminator (i - 1 is the last actual character)
                break;
            }
        }
    
        // Count the amount of deleted characters (the amoount of null-characters after i)
        for (qint64 i = endOfEditIndex; i < file->size(); i++)
        {
            char buffer = '\0';
        
            if (!file->seek(i))
            {
             QString desc("Failed to seek file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        
            if (file->write(&buffer, 1) < 1)
            {
                QString desc("Failed to write to file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }

            if (buffer != '\0')
            {
                // Last null-terminator found
                emptyCharacterCount = i - endOfEditIndex;
                break;
            }
        }

        /* Steps:
         *  1. Write all characters after 'emptyCharacterCount - 1' in file to endOfEditIndex of file
         *  2. Resize the file to the size without the removed characters
         */

        qint64 fileSize = file->size(); // File size changes on overflow, so it needs to be cached here
        char buffer = '\0';
        for (qint64 i = endOfEditIndex; i < fileSize; i++)
        {
            // Read the next non-empty character in the file
            if (!file->seek(endOfEditIndex + emptyCharacterCount))
            {
                QString desc("Failed to seek file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        
            if (file->write(&buffer) < 1)
            {
                QString desc("Failed to write to file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        
            // Write the non-empty character to the next empty character
            if (!file->seek(i))
            {
                QString desc("Failed to seek file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        
            if (file->write(&buffer, 1) < 1)
            {
                QString desc("Failed to write to file '%1'.");
                desc = desc.arg(file->fileName());
            
                sendError("File Error", desc, file->errorString(), file->error());
                return;
            }
        }
    
        if (!file->resize(file->size() - emptyCharacterCount))
        {
            QString desc("Failed to resize file '%1'.");
            desc = desc.arg(file->fileName());
        
            sendError("File Error", desc, file->errorString(), file->error());
            return;
        }
    
    }

    emit finished();
    emit readyForDelete();
}

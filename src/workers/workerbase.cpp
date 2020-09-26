#include "workerbase.h"

#include <QThread>

WorkerBase::WorkerBase(QObject *parent) : QObject(parent)
{
    
}

WorkerBase::~WorkerBase()
{
    
}

void WorkerBase::reportError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line)
{
    emit error(title, description, errorString, errorCode, sourceFile, line);
    thread()->exit(EXIT_FAILURE);
}

void WorkerBase::finishExecution()
{
    emit finished();
    emit readyForDeletion();
    thread()->quit();
}

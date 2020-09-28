#include "workerbase.h"

#include <QThread>

WorkerBase::WorkerBase(QObject *parent) : QObject(parent)
{
    
}

WorkerBase::~WorkerBase()
{
    
}

void WorkerBase::reportError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode)
{
    emit error(title, description, errorString, errorCode);
    thread()->exit(EXIT_FAILURE);
}

void WorkerBase::finishExecution()
{
    emit finished();
    emit readyForDeletion();
    thread()->quit();
}

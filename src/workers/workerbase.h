#ifndef WORKERBASE_H
#define WORKERBASE_H

#include <QObject>

class WorkerBase : public QObject
{
    Q_OBJECT
public:
    explicit WorkerBase(QObject *parent = nullptr);
    virtual ~WorkerBase();
   
protected:
    // Reports an error to the main thread and marks the worker as ready for deletion due to failure.
    // How the error is handled is decided by the main thread using the error(...) signal.
    void reportError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);
    
    // Marks this worker as finished and ready for deletion
    void finishExecution();
    
signals:
    void error(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);
    void finished();
    void readyForDeletion();
    
};

#endif // WORKERBASE_H

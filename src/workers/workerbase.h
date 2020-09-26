#ifndef WORKERBASE_H
#define WORKERBASE_H

#include <QObject>

// Reports an error to the main thread and marks the worker as ready for deletion due to failure.
// How the error is handled is decided by the main thread using the error(...) signal.
#define REPORT_ERROR(title, description, errorString, errorCode) reportError(title, description, errorString, errorCode, __FILE__, __LINE__)

class WorkerBase : public QObject
{
    Q_OBJECT
public:
    explicit WorkerBase(QObject *parent = nullptr);
    virtual ~WorkerBase();
   
protected:
    // WARNING: Use the REPORT_ERROR() macro to report errors instead!
    void reportError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line);
    
    // Marks this worker as finished and ready for deletion
    void finishExecution();
    
signals:
    void error(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line);
    void finished();
    void readyForDeletion();
    
};

#endif // WORKERBASE_H

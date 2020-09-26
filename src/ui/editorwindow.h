#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class EditorWindow; }
QT_END_NAMESPACE

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow();

    void closeEvent(QCloseEvent *event) override;

    void loadBytes(qint64 from, qint64 to);
    void loadBlock(qint64 blockIndex);

    void save(qint64 from);
    
    // Displays an error dialog to the user
    void displayErrorDialog(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line);

    // Returns the amount of bytes in a single block according to the current preferences
    qint64 getBlockSize() const;
    
private:
    Ui::EditorWindow *ui;

    uint64_t m_currentBlock;
    QSharedPointer<QFile> m_currentFile;
    
    bool simpleWrite;
    bool hasUnsavedChanges;

private slots:
    void openAbout();
    void openPreferences();
    void openFile();
    void openSave();
    void openSaveAs();

    void onClickedGoToBlockButton();
    void onBlockSpinBoxValueChanged(int value);

    void onFileReadStarted();
    void onFileReadFinished(qint32 bytesRead, QByteArray* bytes);
    void onFileReadError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line);

    void onFileWriteStarted();
    void onFileWriteFinished();
    void onFileWriteError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode, const QString& sourceFile, qint64 line);

    void onTextEdited(bool modified);
    
    void onPreferencesChanged(bool requireReload);
};
#endif // EDITORWINDOW_H

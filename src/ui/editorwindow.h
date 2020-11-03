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

    void loadFile(const QString& fileName);
    void loadBytes(qint64 from, qint64 to);
    void loadBlock(qint64 chunkIndex);

    void save(qint64 from);
    
    // Displays an error dialog to the user
    void displayErrorDialog(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);

    // Returns the amount of bytes in a single chunk according to the current preferences
    qint64 getBlockSize() const;
    
private:
    Ui::EditorWindow *ui;

    uint64_t currentBlock;
    QSharedPointer<QFile> currentFile;
    
    bool simpleWrite;
    bool hasUnsavedChanges;
    
    // Adds a string value to the recent tab/list on the menubar
    void addRecentFile(const QString& fileName);
    
    void openRecentFile(const QString& fileName);
    
    // Starts a new LFEditor session with the supplied command-line argument(s)
    void startEditorSession(const QList<QString>& args);
    void startEditorSession(const QString& arg);

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
    void onFileReadError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);

    void onFileWriteStarted();
    void onFileWriteFinished();
    void onFileWriteError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode);

    void onTextEdited(bool modified);
    
    void onPreferencesChanged(bool requireReload);
};
#endif // EDITORWINDOW_H

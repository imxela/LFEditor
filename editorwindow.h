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

    void load(quint64 from, quint64 to);
    void loadBlock(quint64 blockIndex);

    void goToBlock(uint64_t blockIndex);

private:
    Ui::EditorWindow *ui;

    uint64_t m_currentBlock;
    QSharedPointer<QFile> m_currentFile;

private slots:
    void openAbout();
    void openPreferences();
    void openFile();

    void onClickedGoToBlockButton();

    void onFileReadStarted();
    void onFileReadFinished(qint32 bytesRead, QByteArray* bytes);

    void onPreferencesChanged();
};
#endif // EDITORWINDOW_H

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
    void about();
    void preferences();
    void open();

    void fileReadStarted();
    void fileReadFinished(qint32 bytesRead, QByteArray* bytes);

    void reloadPreferences(int code);

    void goToBlockButtonClicked();
    void goToNextBlockButtonClicked();
    void goToPreviousBlockButtonClicked();

};
#endif // EDITORWINDOW_H

#include "editorwindow.h"
#include "ui_editorwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QThread>
#include <QSignalMapper>

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"
#include "filereadworker.h"

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EditorWindow)
{
    ui->setupUi(this);

    ui->goToBlockSpinBox->setValue(0); // First block/page

    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(open()));

    connect(ui->goToBlockButton, SIGNAL(clicked()), this, SLOT(goToBlockButtonClicked()));
    connect(ui->nextBlockButton, SIGNAL(clicked()), this, SLOT(goToNextBlockButtonClicked()));
    connect(ui->previousBlockButton, SIGNAL(clicked()), this, SLOT(goToPreviousBlockButtonClicked()));

    ui->fileProgress->setVisible(false);

    reloadPreferences(0); // We have to manually reload preferences here to initialize them

    m_currentBlock = ui->goToBlockSpinBox->value();
}

void EditorWindow::goToBlockButtonClicked()
{
    goToBlock(ui->goToBlockSpinBox->value());
}

void EditorWindow::goToNextBlockButtonClicked()
{
    goToBlock(ui->goToBlockSpinBox->value() + 1);
}

void EditorWindow::goToPreviousBlockButtonClicked()
{
    if (ui->goToBlockSpinBox->value() == 0) return;
    goToBlock(ui->goToBlockSpinBox->value() - 1);
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::about()
{
    QMessageBox::about(this, "About LFEditor", "<b>LFEditor</b> is a Large-File editor, meaning it specializes in viewing and editing large files. "
                                               "Most other text editors are unable to open large files at all, which this program aims to solve. "
                                               "LFEditor was created by Alex Karlsson Lindén and is released under the <b>LGPLv3</b> license. "
                                               "For more info regarding the license, read <a href=\"https://www.gnu.org/licenses/lgpl-3.0.txt\">this</a>. "
                                               "If you wish to view the source code of this software, you can do so <a href=\"https://github.com/alexkarlin/lfeditor\">here</a>."
                                               ""
                                               "<br><br>You can contact me on my personal email: <a href=\"mailto:alexkarlssonlinden@gmail.com\">alexkarlssonlinden@gmail.com</a>");
    QMessageBox::aboutQt(this);
}

void EditorWindow::preferences()
{
    PreferencesDialog preferencesDialog;

    connect(&preferencesDialog, SIGNAL(reloadPreferences(int)), this, SLOT(reloadPreferences(int)));

    preferencesDialog.exec();
}

void EditorWindow::reloadPreferences(int code)
{
    if (code < 0) return; // User didn't save changes, so we don't need to reload

    int wrapModeInt = PreferenceManager::getInstance().wordWrapMode;
    QTextOption::WrapMode wordWrap;

    switch (wrapModeInt)
    {
    case 0:
        wordWrap = QTextOption::WrapMode::NoWrap;
        break;

    case 1:
        wordWrap = QTextOption::WrapMode::WordWrap;
        break;

    case 2:
        wordWrap = QTextOption::WrapMode::WrapAnywhere;
        break;
    case 3:
        wordWrap = QTextOption::WrapMode::WrapAtWordBoundaryOrAnywhere;
        break;

    }

    qDebug() << "Reloading preferences";
    ui->fileEdit->setWordWrapMode(wordWrap);
}

void EditorWindow::open()
{
    // Reset block/page to 0 when opening a new file. Todo: Create a 'setCurrentBlock(quint64)' method for this
    // since it is also used in the constructor for initialization.
    ui->goToBlockSpinBox->setValue(0);
    m_currentBlock = 0;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);

    if (fileDialog.exec())
    {
        QString fileName = fileDialog.selectedFiles()[0];
        setWindowTitle(QString("LFEditor [ %1 ]").arg(fileName));

        m_currentFile = QSharedPointer<QFile>(new QFile(fileName));
        if (!m_currentFile->open(QIODevice::ReadWrite))
        {
            qDebug() << "Failed to open file '" << fileName << "': " << m_currentFile->error();
            QMessageBox::critical(this, "File Error", QString("Failed to load file '%l'.\nReason: %l").arg(fileName, m_currentFile->error()));
            return;
        }

        loadBlock(m_currentBlock);
    }
}

void EditorWindow::load(quint64 from, quint64 to)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    // int blockSize = PreferenceManager::getInstance().blockSize * (1000 / sizeof(char));

    QThread* thread = new QThread(this); // Memory leak? or does connect(...deleteLater()) fix that?
    FileReadWorker* worker = new FileReadWorker();

    worker->moveToThread(thread);

    connect(worker, SIGNAL(finished(qint32, QByteArray*)), this, SLOT(fileReadFinished(qint32, QByteArray*)));
    connect(thread, &QThread::started, worker, [worker, this, from = from, to = to] { worker->readFile(m_currentFile.data(), from, to); } );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(worker, SIGNAL(readyForDelete()), worker, SLOT(deleteLater()));

    fileReadStarted();

    thread->start();
}

void EditorWindow::loadBlock(quint64 blockIndex)
{
    quint64 byteSize = sizeof(char) * PreferenceManager::getInstance().blockSize * (1000 / sizeof(char));
    load(byteSize * blockIndex, byteSize * (blockIndex + 1)); // Load starting at the current block to the next one, aka load 1 block
}

void EditorWindow::fileReadStarted()
{
    qDebug() << "fileReadStarted()";
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();
    ui->fileEdit->setPlaceholderText("Loading file, please wait...");
}

void EditorWindow::fileReadFinished(qint32 bytesRead, QByteArray* bytes)
{

    ui->fileEdit->setPlaceholderText("No file loaded...");

    if (bytesRead >= 0)
    {
        ui->fileEdit->setPlainText(bytes->data()); // This is slow and blocking, ProgressBar needs to animate during this somehow!
    }
    else
    {
        qDebug() << "Failed to read file!";
        QMessageBox::critical(this, "File read error", "Failed to read file!\nReason: Negative bytes read from file.");
    }

    delete bytes; // No longer needed

    qDebug() << "File read finished!";
    ui->fileProgress->setVisible(false);
}

void EditorWindow::goToBlock(uint64_t blockIndex)
{
    ui->goToBlockSpinBox->setValue(blockIndex);
    m_currentBlock = blockIndex;

    qDebug() << "Going to block: " << QString::number(blockIndex);

    loadBlock(blockIndex);
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "LFEditor",
                                                                tr("Are you sure you want to exit?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        PreferenceManager::getInstance().savePreferences();

        event->accept();
    }
}

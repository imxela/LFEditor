#include "editorwindow.h"
#include "ui_editorwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QThread>
#include <QSignalMapper>
#include <QSharedPointer>
#include <QSpinBox>

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"

#include <workers/filereadworker.h>
#include <workers/filewriteworker.h>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EditorWindow), currentBlock(0)
{
    ui->setupUi(this);

    // Note: Saving is not yet implemented, so there's no risk of data loss in the current version.
#if 0
    QMessageBox::warning(this, "Warning - Loss of data", "This software is at an early stage of development. "
                                          "Loss of data is possible and should be expected. "
                                          "Making backups of any and all files you open in this software is therefore highly recommended."
                         );
#endif
    
    hasUnsavedChanges = false;
    
    // Disable all file controls since no file is loaded
    ui->goToChunkSpinBox->setEnabled(false);
    ui->goToChunkButton->setEnabled(false);
    ui->nextChunkButton->setEnabled(false);
    ui->previousChunkButton->setEnabled(false);

    connect(ui->actionExit, &QAction::triggered, this, [this] { this->close(); } );
    connect(ui->actionAbout, &QAction::triggered, this, &EditorWindow::openAbout);
    connect(ui->actionPreferences, &QAction::triggered, this, &EditorWindow::openPreferences);
    connect(ui->actionOpen, &QAction::triggered, this, &EditorWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &EditorWindow::openSave);

    connect(ui->goToChunkButton, &QPushButton::clicked, this, &EditorWindow::onClickedGoToBlockButton);
    connect(ui->goToChunkSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EditorWindow::onBlockSpinBoxValueChanged);
    
    connect(ui->fileEdit, &QPlainTextEdit::modificationChanged, this, &EditorWindow::onTextEdited);

    connect(
        ui->nextChunkButton, &QPushButton::clicked,

        [this]() {
            loadBlock(currentBlock + 1);
        }
    );

    connect(
        ui->previousChunkButton, &QPushButton::clicked,

        [this]() {
            if (currentBlock > 0)
                loadBlock(currentBlock - 1);
        }
    );

    ui->fileProgress->setVisible(false);

    onPreferencesChanged(false); // We have to manually simulate a preference change here to initialize them
    loadBlock(0); // Does some initialization
}

EditorWindow::~EditorWindow()
{
    if (!currentFile.isNull())
        currentFile->close();
    
    delete ui;
}

void EditorWindow::openAbout()
{
    QMessageBox::about(this, "About LFEditor", "<b>LFEditor</b> is a Large-File editor. It specializes in viewing and (not yet) editing large files. "
                                               "Most other text editors are unable to open large files at all, but this software can open files of any size. "
                                               "LFEditor is created by Alex Karlsson Lindén and is released under the <b>LGPLv3</b> license. "
                                               "For more info regarding the license, read <a href=\"https://www.gnu.org/licenses/lgpl-3.0.txt\">this</a>. "
                                               "If you wish to view the source code of this software, you can do so <a href=\"https://github.com/alexkarlin/lfeditor\">here</a>.");
    QMessageBox::aboutQt(this);
}

void EditorWindow::openPreferences()
{
    PreferencesDialog preferencesDialog;

    connect(&preferencesDialog, &PreferencesDialog::onPreferencesChanged, this, &EditorWindow::onPreferencesChanged);

    preferencesDialog.exec();
}

void EditorWindow::openFile()
{
    ui->goToChunkSpinBox->setValue(0);
    currentBlock = 0;

    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);

    if (fileDialog.exec())
    {
        QString fileName = fileDialog.selectedFiles()[0];
        setWindowTitle(QString("LFEditor [ %1 ]").arg(fileName));

        currentFile = QSharedPointer<QFile>(new QFile(fileName));
        if (!currentFile->open(QIODevice::ReadWrite))
        {
            qDebug() << "Failed to open file '" << fileName << "': " << currentFile->error();
            QMessageBox::critical(this, "File Error", QString("Failed to open file '%l'.\nReason: %l").arg(fileName, currentFile->error()));
            return;
        }
        
        // File has been loaded, enable controls
        ui->goToChunkSpinBox->setEnabled(true);
        ui->goToChunkSpinBox->setValue(0); // First chunk/page
        
        ui->goToChunkButton->setEnabled(true);
        ui->nextChunkButton->setEnabled(true);
        ui->previousChunkButton->setEnabled(true);
        
        loadBlock(currentBlock); // Go to the first chunk of the file
    }
}

void EditorWindow::openSave()
{
    save(getBlockSize() * currentBlock);
}

void EditorWindow::openSaveAs()
{

}

void EditorWindow::onClickedGoToBlockButton()
{
    loadBlock(ui->goToChunkSpinBox->value());
}

void EditorWindow::onBlockSpinBoxValueChanged(int value)
{
    qint64 fileIndex = getBlockSize() * value;
    qint64 fileSize = currentFile->size();
    ui->goToChunkButton->setEnabled(fileIndex < fileSize); // Ensure you cannot load a chunk past EOF by disabling the go to chunk button
}

void EditorWindow::onFileReadStarted()
{
    qDebug() << "fileReadStarted()";
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();
    ui->fileEdit->setPlaceholderText("Loading file, please wait...");
}

void EditorWindow::onFileReadFinished(qint32 bytesRead, QByteArray* bytes)
{

    ui->fileEdit->setPlaceholderText("No file loaded...");

    if (bytesRead >= 0)
    {
        ui->fileEdit->setPlainText(bytes->data());
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

void EditorWindow::onFileReadError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode)
{
    displayErrorDialog(title, description, errorString, errorCode);
    
    ui->fileEdit->setPlaceholderText("File read failed!");
    ui->fileProgress->setVisible(false);
}

void EditorWindow::onFileWriteStarted()
{
    qDebug() << "fileWriteStarted()";
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();
}

void EditorWindow::onFileWriteFinished()
{
    ui->fileProgress->setVisible(false);
    hasUnsavedChanges = false; // Changed have been saved
    setWindowTitle(QString("LFEditor [ %1 ]").arg(currentFile->fileName()));
    
    // Reload the current chunk to see the changes made to the file
    loadBlock(currentBlock);
}

void EditorWindow::onFileWriteError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode)
{
    displayErrorDialog(title, description, errorString, errorCode);
    
    ui->fileEdit->setPlaceholderText("File write failed!");
    ui->fileProgress->setVisible(false);
}

void EditorWindow::onTextEdited(bool modified)
{
    if (!currentFile.isNull())
    {
        hasUnsavedChanges = modified;
        
        if (modified)
        {
            setWindowTitle(QString("LFEditor [ %1* ]").arg(currentFile->fileName()));
        }
        else
        {
            setWindowTitle(QString("LFEditor [ %1 ]").arg(currentFile->fileName()));
        }
    }
}

void EditorWindow::onPreferencesChanged(bool requireReload)
{
    qDebug() << "Reloading preferences...";
    
    PreferenceManager& mgr = PreferenceManager::getInstance();
    int wrapModeInt = mgr.wordWrapMode;
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
    
    int writeModeInt = mgr.writeMode;
    simpleWrite = writeModeInt == 0;
    
    ui->fileEdit->setWordWrapMode(wordWrap);
    
    if (requireReload)
    {
        loadBlock(currentBlock);
    }
}

void EditorWindow::loadBytes(qint64 from, qint64 to)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    QThread* thread = new QThread(this);
    FileReadWorker* worker = new FileReadWorker();

    worker->moveToThread(thread);

    connect(worker, &FileReadWorker::error, this, &EditorWindow::onFileReadError);
    connect(worker, &FileReadWorker::result, this, &EditorWindow::onFileReadFinished);
    connect(thread, &QThread::started, worker, [worker, this, from = from, to = to] { worker->readFile(currentFile.data(), from, to); } );
    connect(thread, &QThread::finished, thread, &FileReadWorker::deleteLater);
    connect(worker, &FileReadWorker::readyForDeletion, worker, &FileReadWorker::deleteLater);

    onFileReadStarted();

    thread->start();
}

void EditorWindow::loadBlock(qint64 chunkIndex)
{
    if (currentFile.isNull())
    {
        return;
    }
    
    if (hasUnsavedChanges)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Unsaved Changes",
                                                                tr("You have unsaved changes in your current chunk, are you sure you want to load a new chunk?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::No);
        if (resBtn == QMessageBox::No) {
            return;
        }
    }
    
    ui->goToChunkSpinBox->setValue(chunkIndex);
    currentBlock = chunkIndex;
    
    PreferenceManager mgr = PreferenceManager::getInstance();
    qint64 nextBlockFileIndex = getBlockSize() * (chunkIndex + 1);
    qint64 fileSize = currentFile->size();
    ui->nextChunkButton->setEnabled(nextBlockFileIndex < fileSize); // Ensure you cannot load a chunk past EOF by disabling the next chunk button
    ui->previousChunkButton->setEnabled(currentBlock > 0); // Disable the "previous chunk" button if we are on the first chunk
    
    qint64 chunkByteCount = getBlockSize();
    loadBytes(chunkByteCount * chunkIndex, chunkByteCount * (chunkIndex + 1));
}

void EditorWindow::save(qint64 from)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    QThread* thread = new QThread(this);
    FileWriteWorker* worker = new FileWriteWorker();

    worker->moveToThread(thread);

    connect(worker, &FileWriteWorker::finished, this, &EditorWindow::onFileWriteFinished);
    connect(worker, &FileWriteWorker::error, this, &EditorWindow::onFileWriteError);
    
    QByteArray bytes(ui->fileEdit->toPlainText().toUtf8());
    
    connect(
                thread, &QThread::started, worker, 
                [worker, this, bytes = bytes, from = from, chunkSize = getBlockSize(), simpleWrite = simpleWrite] { 
                    worker->writeFile(currentFile.data(), from, bytes, chunkSize, simpleWrite); 
                } 
    );
    
    connect(thread, &QThread::finished, thread, &FileWriteWorker::deleteLater);
    connect(worker, &FileWriteWorker::readyForDeletion, worker, &FileWriteWorker::deleteLater);

    onFileWriteStarted();

    thread->start();
}

void EditorWindow::displayErrorDialog(const QString &title, const QString &description, const QString &errorString, qint64 errorCode)
{
    // Note: Note sure if I should be displaying things like file and line in non-debug mode.
    // Todo: Maybe add an option to start LFEditor in debug mode to enable this extra information?
    QString text("%1\nReason: %2\nCode: 0x%3");
    text = text.arg(description, errorString, QString::number(errorCode, 16).toUpper());
    QMessageBox::critical(this, title, text);
}

qint64 EditorWindow::getBlockSize() const
{
    PreferenceManager& mgr = PreferenceManager::getInstance();
    return mgr.chunkSize * mgr.byteSize;
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    if (hasUnsavedChanges)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Unsaved Changes",
                                                                tr("You have unsaved changes in your current chunk, are you sure you want to exit?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::No);
        if (resBtn != QMessageBox::Yes) {
            event->ignore();
        } else {
            PreferenceManager::getInstance().savePreferences();
        
            event->accept();
        }
    }
}

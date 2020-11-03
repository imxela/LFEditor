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
#include <QProcess>

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"

#include <workers/filereadworker.h>
#include <workers/filewriteworker.h>

EditorWindow::EditorWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::EditorWindow), currentChunk(0)
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
    
    // Also disable saving the current chunk
    ui->actionSaveChunkAs->setEnabled(false);

    connect(ui->actionExit, &QAction::triggered, this, [this] { this->close(); } );
    connect(ui->actionAbout, &QAction::triggered, this, &EditorWindow::openAbout);
    connect(ui->actionPreferences, &QAction::triggered, this, &EditorWindow::openPreferences);
    connect(ui->actionOpen, &QAction::triggered, this, &EditorWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &EditorWindow::openSave);
    connect(ui->actionSaveChunkAs, &QAction::triggered, this, &EditorWindow::openSaveChunkAs);

    connect(ui->goToChunkButton, &QPushButton::clicked, this, &EditorWindow::onClickedGoToChunkButton);
    connect(ui->goToChunkSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EditorWindow::onChunkSpinBoxValueChanged);
    
    connect(ui->fileEdit, &QPlainTextEdit::modificationChanged, this, &EditorWindow::onTextEdited);

    connect(
        ui->nextChunkButton, &QPushButton::clicked,

        [this]() {
            loadChunk(currentChunk + 1);
        }
    );

    connect(
        ui->previousChunkButton, &QPushButton::clicked,

        [this]() {
            if (currentChunk > 0)
                loadChunk(currentChunk - 1);
        }
    );

    ui->fileProgress->setVisible(false);

    onPreferencesChanged(false); // We have to manually simulate a preference change here to initialize them
    loadChunk(0); // Does some initialization
    
    qDebug() << "Command-line arguments:";
    for (int i = 0; i < QCoreApplication::arguments().count(); i++)
    {
        QString s = QString::number(i+1) + ") " + QCoreApplication::arguments().at(i);
        qDebug() << '\t' << s;
    }
   
    // Load file supplied as command-line argument
    if (QCoreApplication::arguments().count() >= 2)
    {
        QString fileArg = QCoreApplication::arguments().at(1);
        if (!fileArg.isEmpty())
            loadFile(fileArg);
    }
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
                                               "LFEditor is created by <a href=\"https://github.com/alexkarlin\"> @alexkarlin </a> and is released under the <b>LGPLv3</b> license. "
                                               "For more info regarding the license, read <a href=\"https://www.gnu.org/licenses/lgpl-3.0.txt\">this</a>. "
                                               "If you wish to view the source code of this software, you can do so <a href=\"https://github.com/alexkarlin/lfeditor\">here</a>."
                                               "<br><br>"
                                               "If you wish to contact me, you can do so on my email, which is my GitHub username followed by <a href=\"mailto:.dev@gmail.com\"> .dev@gmail.com </a>");
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
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::FileMode::ExistingFiles);

    if (fileDialog.exec())
    {
        // If several files are selected, open all but the first one in new sessions
        if (fileDialog.selectedFiles().count() > 1)
        {
            // Todo: Instead if opening in a new session, create file tabs in a single session
            QList<QString> selection = fileDialog.selectedFiles();
            QList<QString> externalSessionSelections = QList<QString>(selection.begin() + 1, selection.end());
            startEditorSession(externalSessionSelections);
        }
        
        loadFile(fileDialog.selectedFiles().at(0));
    }
}

void EditorWindow::openSave()
{
    save(getChunkSize() * currentChunk);
}

void EditorWindow::openSaveAs()
{

}

void EditorWindow::openSaveChunkAs()
{
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    
    if (fileDialog.exec())
    {
        QFile file(fileDialog.selectedFiles().at(0));
        if (!file.exists())
            file.open(QIODevice::ReadWrite);
                    
        saveChunk(fileDialog.selectedFiles().at(0));
    }
}

void EditorWindow::onClickedGoToChunkButton()
{
    loadChunk(ui->goToChunkSpinBox->value());
}

void EditorWindow::onChunkSpinBoxValueChanged(int value)
{
    qint64 fileIndex = getChunkSize() * value;
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
    loadChunk(currentChunk);
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
    
    for (int i = 0; i < mgr.recentFiles.count(); i++)
    {
        QAction* fileAction = new QAction(mgr.recentFiles[i], ui->menuRecent);
        fileAction->connect(fileAction, &QAction::triggered, this, [this, fileName = fileAction->text()] { this->openRecentFile(fileName); } );
        ui->menuRecent->addAction(fileAction);
    }
    
    if (requireReload)
    {
        loadChunk(currentChunk);
    }
}

void EditorWindow::loadFile(const QString &fileName)
{
    ui->goToChunkSpinBox->setValue(0);
    currentChunk= 0;
    
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
    
    ui->actionSaveChunkAs->setEnabled(true);
    
    addRecentFile(fileName);
    
    loadChunk(currentChunk); // Go to the first chunk of the file
    
    // File has been loaded, enable controls
    ui->goToChunkSpinBox->setEnabled(true);
    ui->goToChunkSpinBox->setValue(0); // First chunk/page
    
    ui->goToChunkButton->setEnabled(true);
    ui->nextChunkButton->setEnabled(true);
    ui->previousChunkButton->setEnabled(true);
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

void EditorWindow::loadChunk(qint64 chunkIndex)
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
    currentChunk= chunkIndex;
    
    PreferenceManager mgr = PreferenceManager::getInstance();
    qint64 nextChunkFileIndex = getChunkSize() * (chunkIndex + 1);
    qint64 fileSize = currentFile->size();
    ui->nextChunkButton->setEnabled(nextChunkFileIndex < fileSize); // Ensure you cannot load a chunk past EOF by disabling the next chunk button
    ui->previousChunkButton->setEnabled(currentChunk> 0); // Disable the "previous chunk" button if we are on the first chunk
    
    qint64 chunkByteCount = getChunkSize();
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
                [worker, this, bytes = bytes, from = from, chunkSize = getChunkSize(), simpleWrite = simpleWrite] { 
                    worker->writeFile(currentFile.data(), from, bytes, chunkSize, simpleWrite); 
                } 
    );
    
    connect(thread, &QThread::finished, thread, &FileWriteWorker::deleteLater);
    connect(worker, &FileWriteWorker::readyForDeletion, worker, &FileWriteWorker::deleteLater);

    onFileWriteStarted();

    thread->start();
}

void EditorWindow::saveChunk(const QString &fileName)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    QThread* thread = new QThread(this);
    FileWriteWorker* worker = new FileWriteWorker();

    worker->moveToThread(thread);

    connect(worker, &FileWriteWorker::finished, this, &EditorWindow::onFileWriteFinished);
    connect(worker, &FileWriteWorker::error, this, &EditorWindow::onFileWriteError);
    
    // Note: Does this run out of scope once this function has finished, or does it ignore that if it has references still?
    QSharedPointer<QFile> chunkFile = QSharedPointer<QFile>(new QFile(fileName));
    
    QByteArray bytes(ui->fileEdit->toPlainText().toUtf8());
    
    connect(
                thread, &QThread::started, worker, 
                [worker, chunkFile, bytes = bytes, from = 0, chunkSize = getChunkSize()] { 
                    worker->writeFile(chunkFile.data(), from, bytes, chunkSize, true); 
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

qint64 EditorWindow::getChunkSize() const
{
    PreferenceManager& mgr = PreferenceManager::getInstance();
    return mgr.chunkSize * mgr.byteSize;
}

void EditorWindow::addRecentFile(const QString& fileName)
{
    // Note: Does using 'new' like this cause a memory leak, or is deletion managed by Qt?
    //       Should be fine, the deletion of the action is managed by the parent,
    //       which is set to 'ui->menuRecent' here below.
    QAction* fileAction = new QAction(fileName, ui->menuRecent);
    
    // Make the action load the file it is associated with when clicked
    fileAction->connect(fileAction, &QAction::triggered, this, [this, fileName] { this->loadFile(fileName); } );
    
    // Only the most recent 5 files are shown in the list, remove last if exceeding
    if (ui->menuRecent->actions().count() > 5)
    {
        ui->menuRecent->removeAction(ui->menuRecent->actions().last());
    }
    
    // Check if the file already exists in the list, if it does,
    // remove it and replace it with the new one on top of the list.
    for (int i = 0; i < ui->menuRecent->actions().count(); i++)
    {
        if (ui->menuRecent->actions()[i]->text() == fileAction->text())
        {
            ui->menuRecent->removeAction(ui->menuRecent->actions()[i]);
        }
    }
    
    // Add file to recent menu
    // If there are no recent files except the current one, we do not need to insert in front
    if (ui->menuRecent->actions().count() == 0)
        ui->menuRecent->addAction(fileAction);
    else
        ui->menuRecent->insertAction(ui->menuRecent->actions().first(), fileAction);
    
    // Save the recent files menu to preferences so they're saved between sessions
    PreferenceManager& mgr = PreferenceManager::getInstance();
    QList<QString> recentFileStrings = QList<QString>();
    for (int i = 0; i < ui->menuRecent->actions().count(); i++)
        recentFileStrings.append(ui->menuRecent->actions()[i]->text());
        
    mgr.recentFiles = recentFileStrings;
}

void EditorWindow::openRecentFile(const QString& fileName)
{
    // If control is pressed, open 'fileName' in a new session
    if (QGuiApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        startEditorSession(fileName);
    else
        loadFile(fileName);
}

void EditorWindow::startEditorSession(const QList<QString>& args)
{
    if (!QProcess::startDetached(QCoreApplication::arguments().at(0), args))
    {
        displayErrorDialog("LFEditor Launch Error", "Failed to start a new session of LFEditor", "Cannot get error information from detached processes", 0);
    }
}

void EditorWindow::startEditorSession(const QString& arg)
{
    QList<QString> list = QList<QString>();
    list.append(arg);
    
    if (!QProcess::startDetached(QCoreApplication::arguments().at(0), list))
    {
        displayErrorDialog("LFEditor Launch Error", "Failed to start a new session of LFEditor", "Cannot get error information from detached processes", 0);
    }
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    PreferenceManager::getInstance().savePreferences();
    
    if (hasUnsavedChanges)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Unsaved Changes",
                                                                tr("You have unsaved changes in your current chunk, are you sure you want to exit?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::No);
        if (resBtn != QMessageBox::Yes) {
            event->ignore();
        } else {
            event->accept();
        }
    }
}

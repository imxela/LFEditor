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
    , ui(new Ui::EditorWindow), m_currentBlock(0)
{
    ui->setupUi(this);

    QMessageBox::warning(this, "Warning - Loss of data", "This software is at an early stage of development. "
                                          "Loss of data is possible and should be expected. "
                                          "Making backups of any and all files you open in this software is therefore highly recommended."
                         );
    
    hasUnsavedChanges = false;
    
    // Disable all file controls since no file is loaded
    ui->goToBlockSpinBox->setEnabled(false);
    ui->goToBlockButton->setEnabled(false);
    ui->nextBlockButton->setEnabled(false);
    ui->previousBlockButton->setEnabled(false);

    connect(ui->actionExit, &QAction::triggered, this, [this] { this->close(); } );
    connect(ui->actionAbout, &QAction::triggered, this, &EditorWindow::openAbout);
    connect(ui->actionPreferences, &QAction::triggered, this, &EditorWindow::openPreferences);
    connect(ui->actionOpen, &QAction::triggered, this, &EditorWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &EditorWindow::openSave);

    connect(ui->goToBlockButton, &QPushButton::clicked, this, &EditorWindow::onClickedGoToBlockButton);
    connect(ui->goToBlockSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &EditorWindow::onBlockSpinBoxValueChanged);
    
    connect(ui->fileEdit, &QPlainTextEdit::modificationChanged, this, &EditorWindow::onTextEdited);

    connect(
        ui->nextBlockButton, &QPushButton::clicked,

        [this]() {
            loadBlock(m_currentBlock + 1);
        }
    );

    connect(
        ui->previousBlockButton, &QPushButton::clicked,

        [this]() {
            if (m_currentBlock > 0)
                loadBlock(m_currentBlock - 1);
        }
    );

    ui->fileProgress->setVisible(false);

    onPreferencesChanged(false); // We have to manually simulate a preference change here to initialize them
    loadBlock(0); // Does some initialization
}

EditorWindow::~EditorWindow()
{
    if (!m_currentFile.isNull())
        m_currentFile->close();
    
    delete ui;
}

void EditorWindow::openAbout()
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

void EditorWindow::openPreferences()
{
    PreferencesDialog preferencesDialog;

    connect(&preferencesDialog, &PreferencesDialog::onPreferencesChanged, this, &EditorWindow::onPreferencesChanged);

    preferencesDialog.exec();
}

void EditorWindow::openFile()
{
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
            QMessageBox::critical(this, "File Error", QString("Failed to open file '%l'.\nReason: %l").arg(fileName, m_currentFile->error()));
            return;
        }
        
        // File has been loaded, enable controls
        ui->goToBlockSpinBox->setEnabled(true);
        ui->goToBlockSpinBox->setValue(0); // First block/page
        
        ui->goToBlockButton->setEnabled(true);
        ui->nextBlockButton->setEnabled(true);
        ui->previousBlockButton->setEnabled(true);
        
        loadBlock(m_currentBlock); // Go to the first block of the file
    }
}

void EditorWindow::openSave()
{
    save(getBlockSize() * m_currentBlock);
}

void EditorWindow::openSaveAs()
{

}

void EditorWindow::onClickedGoToBlockButton()
{
    loadBlock(ui->goToBlockSpinBox->value());
}

void EditorWindow::onBlockSpinBoxValueChanged(int value)
{
    qint64 fileIndex = getBlockSize() * value;
    qint64 fileSize = m_currentFile->size();
    ui->goToBlockButton->setEnabled(fileIndex < fileSize); // Ensure you cannot load a block past EOF by disabling the go to block button
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
    setWindowTitle(QString("LFEditor [ %1 ]").arg(m_currentFile->fileName()));
    
    // Reload the current block to see the changes made to the file
    loadBlock(m_currentBlock);
}

void EditorWindow::onFileWriteError(const QString& title, const QString& description, const QString& errorString, qint64 errorCode)
{
    QString text("%1\nReason: %2\nCode: 0x%3");
    text = text.arg(description, errorString, QString::number(errorCode, 16).toUpper());
    QMessageBox::critical(this, title, text);
}

void EditorWindow::onTextEdited(bool modified)
{
    if (!m_currentFile.isNull())
    {
        hasUnsavedChanges = modified;
        
        if (modified)
        {
            setWindowTitle(QString("LFEditor [ %1* ]").arg(m_currentFile->fileName()));
        }
        else
        {
            setWindowTitle(QString("LFEditor [ %1 ]").arg(m_currentFile->fileName()));
        }
    }
}

void EditorWindow::onPreferencesChanged(bool requireReload)
{
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

    qDebug() << "Reloading preferences";
    ui->fileEdit->setWordWrapMode(wordWrap);
    
    if (requireReload)
    {
        loadBlock(m_currentBlock);
    }
}

void EditorWindow::loadBytes(qint64 from, qint64 to)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    QThread* thread = new QThread(this); // Memory leak? or does connect(...deleteLater()) fix that?
    FileReadWorker* worker = new FileReadWorker();

    worker->moveToThread(thread);

    connect(worker, &FileReadWorker::finished, this, &EditorWindow::onFileReadFinished);
    connect(thread, &QThread::started, worker, [worker, this, from = from, to = to] { worker->readFile(m_currentFile.data(), from, to); } );
    connect(thread, &QThread::finished, thread, &FileReadWorker::deleteLater);
    connect(worker, &FileReadWorker::readyForDelete, worker, &FileReadWorker::deleteLater);

    onFileReadStarted();

    thread->start();
}

void EditorWindow::loadBlock(qint64 blockIndex)
{
    if (m_currentFile.isNull())
    {
        // m_currentBlock = 0;
        // ui->goToBlockSpinBox->setValue(0);
        return;
    }
    
    if (hasUnsavedChanges)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Unsaved Changes",
                                                                tr("You have unsaved changes in your current block, are you sure you want to load a new block?\n"),
                                                                QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::No);
        if (resBtn == QMessageBox::No) {
            return;
        }
    }
    
    ui->goToBlockSpinBox->setValue(blockIndex);
    m_currentBlock = blockIndex;
    
    PreferenceManager mgr = PreferenceManager::getInstance();
    qint64 nextBlockFileIndex = getBlockSize() * (blockIndex + 1);
    qint64 fileSize = m_currentFile->size();
    ui->nextBlockButton->setEnabled(nextBlockFileIndex < fileSize); // Ensure you cannot load a block past EOF by disabling the next block button
    ui->previousBlockButton->setEnabled(m_currentBlock > 0); // Disable the "previous block" button if we are on the first block
    
    qint64 blockByteCount = getBlockSize();
    loadBytes(blockByteCount * blockIndex, blockByteCount * (blockIndex + 1));
}

void EditorWindow::save(qint64 from)
{
    ui->fileProgress->setVisible(true);
    ui->fileProgress->reset();

    QThread* thread = new QThread(this); // Memory leak? or does connect(...deleteLater()) fix that?
    FileWriteWorker* worker = new FileWriteWorker();

    worker->moveToThread(thread);

    connect(worker, &FileWriteWorker::finished, this, &EditorWindow::onFileWriteFinished);
    connect(worker, &FileWriteWorker::error, this, &EditorWindow::onFileWriteError);
    
    QByteArray bytes(ui->fileEdit->toPlainText().toUtf8());
    
    connect(
                thread, &QThread::started, worker, 
                [worker, this, bytes = bytes, from = from, blockSize = getBlockSize(), simpleWrite = simpleWrite] { 
                    worker->writeFile(m_currentFile.data(), from, bytes, blockSize, simpleWrite); 
                } 
    );
    
    connect(thread, &QThread::finished, thread, &FileWriteWorker::deleteLater);
    connect(worker, &FileWriteWorker::readyForDelete, worker, &FileWriteWorker::deleteLater);

    onFileWriteStarted();

    thread->start();
}

qint64 EditorWindow::getBlockSize() const
{
    PreferenceManager& mgr = PreferenceManager::getInstance();
    return mgr.blockSize * mgr.byteSize;
}

void EditorWindow::closeEvent(QCloseEvent *event)
{
    if (hasUnsavedChanges)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Unsaved Changes",
                                                                tr("You have unsaved changes in your current block, are you sure you want to exit?\n"),
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

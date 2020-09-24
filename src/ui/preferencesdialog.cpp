#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"

#include <QMessageBox>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    connect(ui->saveButton, &QPushButton::clicked, this, &PreferencesDialog::onClickedSaveButton);
    connect(ui->cancelButton, &QPushButton::clicked, this, &PreferencesDialog::onClickedCancelButton);

    PreferenceManager& mgr = PreferenceManager::getInstance();
    ui->blockSizeSpinBox->setValue(mgr.blockSize);
    ui->wrapModeComboBox->setCurrentIndex(mgr.wordWrapMode);
    ui->writeModeComboBox->setCurrentIndex(mgr.writeMode);
    
    connect(ui->writeModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) { 
        if (index == 0)
        {
            QMessageBox::warning(this, "Warning", "The simple file-saving mode cannot delete characters from a file. "
                                                  "Instead, any deleted characters are replaced by blank spaces. "
                                                  "This is because removing characters requires the file to be copied, "
                                                  "which can take a substantial amount of time if the file is large."
                                                  ""
                                                  "\n\nIf deleting characters is a must, use complex file-saving instead.");
        }
        else if (index == 1)
        {
            QMessageBox::warning(this, "Warning", "Complex file-saving has not yet been implemented and will not work!");
        }
    });

    // Disables the help (?) next to the close button (X), not sure if this can be done directly on QtDesigner.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::onClickedSaveButton()
{
    PreferenceManager& mgr = PreferenceManager::getInstance();
    mgr.blockSize = ui->blockSizeSpinBox->value();
    mgr.wordWrapMode = ui->wrapModeComboBox->currentIndex();
    mgr.writeMode = ui->writeModeComboBox->currentIndex();

    emit onPreferencesChanged();

    close();
}

void PreferencesDialog::onClickedCancelButton()
{
    close();
}

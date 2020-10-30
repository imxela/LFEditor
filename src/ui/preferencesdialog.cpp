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
    ui->chunkSizeSpinBox->setValue(mgr.chunkSize);
    ui->wrapModeComboBox->setCurrentIndex(mgr.wordWrapMode);
    ui->writeModeComboBox->setCurrentIndex(mgr.writeMode);
    ui->byteSizeComboBox->setCurrentIndex(mgr.byteSizeIndex);
    
    // Setup item data for each size
    ui->byteSizeComboBox->setItemData(0, uint(1));
    ui->byteSizeComboBox->setItemData(1, uint(1000));
    ui->byteSizeComboBox->setItemData(2, uint(1000000));
    ui->byteSizeComboBox->setItemData(3, uint(1*10^+9));
    
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
            // Todo: Enable this again once file-saving has been implemented.
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
    
    // If the chunk size has changed in any way, the current chunk has to be reloaded to reflect the changes
    bool reloadBlock = mgr.chunkSize != ui->chunkSizeSpinBox->value() 
                    || mgr.byteSizeIndex != ui->byteSizeComboBox->currentIndex();
    
    mgr.chunkSize = ui->chunkSizeSpinBox->value();
    mgr.wordWrapMode = ui->wrapModeComboBox->currentIndex();
    mgr.writeMode = ui->writeModeComboBox->currentIndex();
    mgr.byteSizeIndex = ui->byteSizeComboBox->currentIndex();
    mgr.byteSize = ui->byteSizeComboBox->itemData(mgr.byteSizeIndex).toUInt();
    
    mgr.savePreferences();
    
    emit onPreferencesChanged(reloadBlock);

    close();
}

void PreferencesDialog::onClickedCancelButton()
{
    close();
}

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    connect(ui->saveButton, &QPushButton::clicked, this, &PreferencesDialog::onClickedSaveButton);
    connect(ui->cancelButton, &QPushButton::clicked, this, &PreferencesDialog::onClickedCancelButton);

    ui->blockSizeSpinBox->setValue(PreferenceManager::getInstance().blockSize);
    ui->wrapModeComboBox->setCurrentIndex(PreferenceManager::getInstance().wordWrapMode);

    // Disables the help (?) next to the close button (X), not sure if this can be done directly on QtDesigner.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::onClickedSaveButton()
{
    PreferenceManager::getInstance().blockSize = ui->blockSizeSpinBox->value();
    PreferenceManager::getInstance().wordWrapMode = ui->wrapModeComboBox->currentIndex();

    emit onPreferencesChanged();

    close();
}

void PreferencesDialog::onClickedCancelButton()
{
    close();
}

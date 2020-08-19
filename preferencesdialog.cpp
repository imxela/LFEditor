#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "preferencemanager.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(btnSave()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(btnCancel()));

    ui->blockSizeSpinBox->setValue(PreferenceManager::getInstance().blockSize);
    ui->wrapModeComboBox->setCurrentIndex(PreferenceManager::getInstance().wordWrapMode);

    // Disables the help (?) next to the close button (X), not sure if this can be done directly on QtDesigner.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::btnSave()
{
    PreferenceManager::getInstance().blockSize = ui->blockSizeSpinBox->value();
    PreferenceManager::getInstance().wordWrapMode = ui->wrapModeComboBox->currentIndex();

    emit reloadPreferences(0);

    close();
}

void PreferencesDialog::btnCancel()
{
    emit reloadPreferences(-1);

    close();
}

#ifndef IMPORTOPTIONSDIALOG_H
#define IMPORTOPTIONSDIALOG_H

#include "otfimportoptions.h"
#include <QDialog>

namespace Ui {
class ImportOptionsDialog;
}

class ImportOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportOptionsDialog(QWidget *parent = 0, OTFImportOptions * _options = NULL);
    ~ImportOptionsDialog();

public slots:
    void onCancel();
    void onOK();
    void onPartitionByFunction(bool value);
    void onPartitionByHeuristic(bool value);
    void onWaitallMerge(bool merge);
    void onLeapMerge(bool merge);
    void onLeapSkip(bool skip);
    void onFunctionEdit(const QString& text);


private:
    Ui::ImportOptionsDialog *ui;

    OTFImportOptions * options;
    OTFImportOptions saved;

    void setUIState();
};

#endif // IMPORTOPTIONSDIALOG_H
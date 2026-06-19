#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

class LicenseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LicenseDialog(QWidget* parent = nullptr);

private slots:
    void onAgree();
    void onDisagree();

private:
    void setupUI();
    void loadLicenseText();
    void reject() override;

    QTextEdit* textEdit;
    QPushButton* btnAgree;
    QPushButton* btnDisagree;
    QLabel* infoLabel;
    bool        agreed;
};

#endif
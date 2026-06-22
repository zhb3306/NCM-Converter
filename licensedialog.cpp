/*
 * NCM Converter - A GUI tool.
 * Copyright (C) 2026 ZHB3306
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "licensedialog.h"
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QMessageBox>
#include <QTimer>

LicenseDialog::LicenseDialog(QWidget* parent)
    : QDialog(parent), agreed(false)
{
    setWindowTitle("许可 - GNU General Public License");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(600, 500);

    setupUI();
    loadLicenseText();

    infoLabel->setText("本软件遵循 GPL-3.0 协议，为了保证不被违规分发，请仔细阅读并同意该许可协议。\n"
        "此外，本软件不需要联网，不会也无法收集用户信息，请放心使用。");
    btnAgree->setText("我同意");
    btnDisagree->setText("我不同意");

    connect(btnAgree, &QPushButton::clicked, this, &LicenseDialog::onAgree);
    connect(btnDisagree, &QPushButton::clicked, this, &LicenseDialog::onDisagree);
}

void LicenseDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel);

    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    mainLayout->addWidget(textEdit);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnAgree = new QPushButton(this);
    btnDisagree = new QPushButton(this);
    btnLayout->addStretch();
    btnLayout->addWidget(btnAgree);
    btnLayout->addWidget(btnDisagree);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void LicenseDialog::loadLicenseText()
{
    QFile file(":/gpl-3.0.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        textEdit->setPlainText(stream.readAll());
        file.close();
    }
    else {
        textEdit->setPlainText("GPL-3.0 协议文本未找到，请检查资源文件。");
    }
    textEdit->moveCursor(QTextCursor::Start);
}

void LicenseDialog::onAgree()
{
    QMessageBox confirmBox(QMessageBox::Question, "许可 - GNU General Public License",
        "你确定吗？\n确定请单击“是”，否则请单击“否”",
        QMessageBox::NoButton, this);
    QPushButton* yesBtn = confirmBox.addButton("是", QMessageBox::YesRole);
    QPushButton* noBtn = confirmBox.addButton("否", QMessageBox::NoRole);
    confirmBox.exec();

    if (confirmBox.clickedButton() == yesBtn) {
        agreed = true;
        accept();
    }
    else {
        onDisagree();
    }
}

void LicenseDialog::onDisagree()
{
    reject();
}

void LicenseDialog::reject()
{
    QDialog exitDialog(this);
    exitDialog.setWindowTitle("许可 - GNU General Public License");
    exitDialog.setWindowFlags(exitDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    exitDialog.setFixedSize(400, 150);
    QVBoxLayout* layout = new QVBoxLayout(&exitDialog);
    QLabel* msgLabel = new QLabel("由于没有同意许可，软件将退出。", &exitDialog);
    msgLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(msgLabel);

    QPushButton* exitBtn = new QPushButton("现在就退出 (5)", &exitDialog);
    layout->addWidget(exitBtn);

    int countdown = 5;
    QTimer timer;
    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        countdown--;
        if (countdown <= 0) {
            timer.stop();
            exitDialog.accept();
            QApplication::quit();
        }
        else {
            exitBtn->setText(QString("现在就退出 (%1)").arg(countdown));
        }
        });
    QObject::connect(exitBtn, &QPushButton::clicked, [&]() {
        timer.stop();
        exitDialog.accept();
        QApplication::quit();
        });

    timer.start();
    exitDialog.exec();
    QApplication::quit();
}
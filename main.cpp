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
#include "ncmdumpGUIwithoutgo.h"
#include "licensedialog.h"
#include "splashscreen.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QTextCodec>
#include <QIcon>
#include <QSettings>
#include <QTimer>
#include <QElapsedTimer>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMessageBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <windows.h>
#include <shellapi.h>
#include <QDir>

void associateFileType()
{
    // ----- 1. 动态获取当前程序路径，并规范化为 Windows 格式 -----
    QString appPath = QCoreApplication::applicationDirPath();
    QString exePath = QDir::toNativeSeparators(appPath + "/NCM Converter.exe");
    QString iconPath = QDir::toNativeSeparators(appPath + "/icons/NCM_File.ico");

    // 如果图标文件不存在，回退到 exe 图标
    if (!QFile::exists(iconPath)) {
        qDebug() << "＞︿＜ 图标文件不存在，使用 exe 图标：" << iconPath;
        iconPath = exePath;
    }
    else {
        qDebug() << "(❁´◡`❁) 图标文件找到：" << iconPath;
    }

    if (!QFile::exists(exePath)) {
        qDebug() << " ＞︿＜ 错误：exe 文件不存在" << exePath;
        return;
    }


    QSettings ext("HKEY_CURRENT_USER\\Software\\Classes\\.ncm", QSettings::NativeFormat);
    ext.setValue("Default", "NCMConverter.ncm");
    ext.sync();

    QSettings prog("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm", QSettings::NativeFormat);
    prog.setValue("Default", "NCM 加密音乐文件");
    prog.setValue("FriendlyAppName", "NCM Converter");
    prog.sync();

    QSettings app("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm\\Application", QSettings::NativeFormat);
    app.setValue("Default", "NCM Converter.exe");
    app.setValue("ApplicationName", "NCM Converter");
    app.setValue("ApplicationIcon", iconPath + ",0");
    app.sync();

    QSettings icon("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm\\DefaultIcon", QSettings::NativeFormat);
    icon.setValue("Default", iconPath + ",0");
    icon.sync();

    QSettings cmd("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm\\shell\\open\\command", QSettings::NativeFormat);
    cmd.setValue("Default", "\"" + exePath + "\" \"%1\"");
    cmd.sync();

    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL);

    qDebug() << "已写入：";
    qDebug() << "  .ncm -> NCMConverter.ncm";
    qDebug() << "  ProgID：NCM Converter";
    qDebug() << "  路径：" << iconPath << ",0";
    qDebug() << "  命令：" << "\"" + exePath + "\" \"%1\"";
}

static bool isAlreadyRunning()
{
    QLocalSocket socket;
    socket.connectToServer("NCMConverterServer");
    if (socket.waitForConnected(500)) {
        QStringList args = QCoreApplication::arguments();
        if (args.size() > 1) {
            QByteArray data = args.last().toUtf8();
            socket.write(data);
            socket.flush();
            socket.waitForBytesWritten(500);
        }
        socket.close();
        return true;
    }
    return false;
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("NCM Converter");
    a.setOrganizationName("ZHB STUDIOS");
    a.setWindowIcon(QIcon(":/icons/NCMC.ico"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    QString fontPath = QCoreApplication::applicationDirPath() + "/fonts/SourceHanSansSC-Bold-2.otf";
    QFont font;
    bool fontLoaded = false;
    if (QFile::exists(fontPath)) {
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                font.setFamily(families.first());
                font.setWeight(QFont::Bold);
                fontLoaded = true;
            }
        }
    }
    if (!fontLoaded) {
        font.setFamily("Source Han Sans SC");
        font.setWeight(QFont::Bold);
    }
    font.setPointSize(10);
    a.setFont(font);
    if (!font.family().isEmpty()) {
        a.setStyleSheet(QString("* { font-family: \"%1\"; font-weight: bold; }").arg(font.family()));
    }
    else {
        a.setStyleSheet("* { font-weight: bold; }");
    }

    if (isAlreadyRunning()) {
        return 0;
    }

    QLocalServer server;
    if (!server.listen("NCMConverterServer")) {
        QLocalServer::removeServer("NCMConverterServer");
        server.listen("NCMConverterServer");
    }

    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    bool agreed = settings.value("AGE", false).toBool();

    if (!agreed) {
        LicenseDialog licenseDlg;
        if (licenseDlg.exec() != QDialog::Accepted) {
            return 0;
        }
        settings.setValue("AGE", true);
        settings.sync();

        QDialog choiceDialog;
        choiceDialog.setWindowTitle("NCM Converter");
        choiceDialog.setWindowFlags(choiceDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
        QVBoxLayout* layout = new QVBoxLayout(&choiceDialog);
        QLabel* label = new QLabel("以后想双击 .ncm 文件后执行什么操作？", &choiceDialog);
        layout->addWidget(label);

        QRadioButton* radioConvert = new QRadioButton("转换（打开 NCM Converter）", &choiceDialog);
        QRadioButton* radioPlay = new QRadioButton("播放（打开网易云音乐）", &choiceDialog);
        radioConvert->setChecked(true);
        layout->addWidget(radioConvert);
        layout->addWidget(radioPlay);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &choiceDialog);
        layout->addWidget(buttonBox);

        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &choiceDialog, &QDialog::accept);

        if (choiceDialog.exec() == QDialog::Accepted) {
            if (radioConvert->isChecked()) {
                settings.setValue("DNC", "YES");
                associateFileType();
            }
            else {
                settings.setValue("DNC", "NO");
            }
            settings.sync();
        }
        else {
            settings.setValue("DNC", "NO");
            settings.sync();
        }
    }

    SplashScreen splash;
    splash.show();

    QElapsedTimer timer;
    timer.start();

    ncmdumpGUIwithoutgo w;
    w.initialize();

    splash.setStatus("加载完成");
    int elapsed = timer.elapsed();
    int remaining = 3000 - elapsed;
    if (remaining < 0) remaining = 0;

    QTimer::singleShot(remaining, [&]() {
        splash.close();
        w.show();

        QStringList args = QCoreApplication::arguments();
        if (args.size() > 1) {
            QString filePath = args.last();
            if (QFile::exists(filePath) && filePath.endsWith(".ncm", Qt::CaseInsensitive)) {
                w.addFiles(QStringList() << filePath);
            }
        }

        QObject::connect(&server, &QLocalServer::newConnection, [&]() {
            QLocalSocket* client = server.nextPendingConnection();
            if (client) {
                if (client->waitForReadyRead(1000)) {
                    QString filePath = QString::fromUtf8(client->readAll());
                    if (QFile::exists(filePath) && filePath.endsWith(".ncm", Qt::CaseInsensitive)) {
                        w.addFiles(QStringList() << filePath);
                    }
                }
                client->deleteLater();
            }
            });
        });

    return a.exec();
}
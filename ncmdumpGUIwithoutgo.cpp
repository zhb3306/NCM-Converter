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
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QDialog>
#include <QTextEdit>
#include <QPixmap>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <windows.h>
#include <eh.h>

static int SafeCallDll(const char* utf8Path, const char* outputDirUtf8,
    CreateCryptFunc create, DumpFunc dump,
    FixMetadataFunc fix, DestroyCryptFunc destroy)
{
    __try {
        void* crypt = create(utf8Path);
        if (!crypt) return -2;
        int dumpResult = dump(crypt, outputDirUtf8);
        if (dumpResult != 0) {
            destroy(crypt);
            return -3;
        }
        fix(crypt);
        destroy(crypt);
        return 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -5;
    }
}

ncmdumpGUIwithoutgo::ncmdumpGUIwithoutgo(QWidget* parent)
    : QMainWindow(parent), currentMp3Process(nullptr), totalFiles(0), processedCount(0), keepFlacAfterMp3(false), dllLoaded(false)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("NCM Converter");

    QMenuBar* menuBar = new QMenuBar(this);
    mainMenu = new QMenu("NCM Converter", this);
    actionSettings = new QAction("设置", this);
    actionAbout = new QAction("关于", this);
    mainMenu->addAction(actionSettings);
    mainMenu->addAction(actionAbout);
    menuBar->addMenu(mainMenu);
    setMenuBar(menuBar);

    connect(actionSettings, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onSettings);
    connect(actionAbout, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onAbout);

    QWidget* central = new QWidget(this);
    central->setObjectName("centralWidget");
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    fileListWidget = new QListWidget(this);
    fileListWidget->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(fileListWidget);

    QHBoxLayout* btnLayout1 = new QHBoxLayout;
    btnAddFiles = new QPushButton("添加文件", this);
    btnAddFolder = new QPushButton("添加文件夹", this);
    btnClear = new QPushButton("清空列表", this);
    btnDelete = new QPushButton("删除", this);
    btnLayout1->addWidget(btnAddFiles);
    btnLayout1->addWidget(btnAddFolder);
    btnLayout1->addWidget(btnClear);
    btnLayout1->addWidget(btnDelete);
    mainLayout->addLayout(btnLayout1);

    QHBoxLayout* outputLayout = new QHBoxLayout;
    QLabel* outputLabel = new QLabel("输出目录:", this);
    outputDirEdit = new QLineEdit(this);
    outputDirEdit->setPlaceholderText("留空表示与源文件同目录，也可手动输入或选择");
    btnSelectOutput = new QPushButton("浏览...", this);
    outputLayout->addWidget(outputLabel);
    outputLayout->addWidget(outputDirEdit);
    outputLayout->addWidget(btnSelectOutput);
    mainLayout->addLayout(outputLayout);

    QHBoxLayout* convertLayout = new QHBoxLayout;
    btnConvert = new QPushButton("开始转换", this);
    checkBoxToMp3 = new QCheckBox("转换为MP3", this);
    checkBoxKeepFlac = new QCheckBox("也保留FLAC文件", this);
    convertLayout->addWidget(btnConvert);
    convertLayout->addWidget(checkBoxToMp3);
    convertLayout->addWidget(checkBoxKeepFlac);
    mainLayout->addLayout(convertLayout);

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setRange(0, 100);
    mainLayout->addWidget(progressBar);

    logEdit = new QTextEdit(this);
    logEdit->setReadOnly(true);
    mainLayout->addWidget(logEdit);

    setCentralWidget(central);
    resize(800, 600);

    QString styleSheet =
        "QMainWindow { background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "   stop:0 #c0d9ff, stop:0.35 #e0c8ff, stop:0.7 #ffe6c7, stop:1 #b8e1fc); }"
        "QWidget#centralWidget {"
        "   background: rgba(255, 255, 255, 0.18);"
        "   border-radius: 30px;"
        "   border: 1px solid rgba(255, 255, 255, 0.4);"
        "}"
        "QWidget { color: #2b2f4c; }"
        "QPushButton {"
        "   background: rgba(255, 255, 255, 0.5);"
        "   color: #2b2f4c;"
        "   border: 1px solid rgba(255, 255, 255, 0.6);"
        "   border-radius: 30px;"
        "   padding: 8px 20px;"
        "   font-weight: 600;"
        "   font-size: 13px;"
        "}"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.7); }"
        "QPushButton:pressed { background: rgba(200, 200, 255, 0.5); }"
        "QPushButton:disabled { background: rgba(200, 200, 200, 0.3); color: #888; }"
        "QListWidget {"
        "   background: rgba(255, 255, 255, 0.4);"
        "   border: 1px solid rgba(255, 255, 255, 0.5);"
        "   border-radius: 20px;"
        "   padding: 5px;"
        "   color: #2b2f4c;"
        "}"
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:selected { background: rgba(159, 139, 203, 0.3); }"
        "QLineEdit {"
        "   background: rgba(255, 255, 255, 0.4);"
        "   border: 1px solid rgba(255, 255, 255, 0.5);"
        "   border-radius: 20px;"
        "   padding: 6px 14px;"
        "   color: #2b2f4c;"
        "}"
        "QLineEdit:focus { border: 1px solid #9f8bcb; }"
        "QTextEdit {"
        "   background: rgba(255, 255, 255, 0.3);"
        "   border: 1px solid rgba(255, 255, 255, 0.4);"
        "   border-radius: 20px;"
        "   padding: 8px;"
        "   color: #2b2f4c;"
        "}"
        "QProgressBar {"
        "   border: none;"
        "   background: rgba(255, 255, 255, 0.4);"
        "   border-radius: 20px;"
        "   text-align: center;"
        "   color: #2b2f4c;"
        "   height: 12px;"
        "}"
        "QProgressBar::chunk {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "       stop:0 #ffb7c5, stop:0.5 #a3ddff, stop:1 #c5a3ff);"
        "   border-radius: 20px;"
        "}"
        "QCheckBox { color: #2b2f4c; font-weight: 500; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QLabel { color: #2b2f4c; font-weight: 500; }"
        "QMenu{ background: rgba(255,255,255,0.85); border: 0px solid transparent; outline: none; border - radius: 15px; margin: 0px; padding: 5px; }"
        "QMenu::item{background: transparent; padding: 6px 20px;}"
        "QMenu::item:selected{background: rgba(159,139,203,0.3); border - radius: 10px;}ted{background: rgba(159,139,203,0.3); border - radius: 10px;}"
        "QDialog { background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "   stop:0 #c0d9ff, stop:0.5 #e0c8ff, stop:1 #ffe6c7); }"
        "QDialog QLabel { color: #2b2f4c; }"
        "QDialog QPushButton {"
        "   background: rgba(255,255,255,0.6);"
        "   border: 1px solid rgba(255,255,255,0.7);"
        "   border-radius: 30px;"
        "   padding: 6px 18px;"
        "   color: #2b2f4c;"
        "   font-weight: 600;"
        "}"
        "QDialog QPushButton:hover { background: rgba(255,255,255,0.8); }"
        "QDialog QTextEdit {"
        "   background: rgba(255,255,255,0.4);"
        "   border: 1px solid rgba(255,255,255,0.5);"
        "   border-radius: 20px;"
        "}"
        "QDialog QLineEdit {"
        "   background: rgba(255,255,255,0.4);"
        "   border: 1px solid rgba(255,255,255,0.5);"
        "   border-radius: 20px;"
        "}";

    setStyleSheet(styleSheet);
    menuBar->setStyleSheet("background: transparent; color: #2b2f4c;");

    connect(btnAddFiles, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onAddFiles);
    connect(btnAddFolder, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onAddFolder);
    connect(btnClear, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onClearFiles);
    connect(btnDelete, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onDeleteSelected);
    connect(btnSelectOutput, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onSelectOutputDir);
    connect(btnConvert, &QPushButton::clicked, this, &ncmdumpGUIwithoutgo::onConvert);
}

ncmdumpGUIwithoutgo::~ncmdumpGUIwithoutgo()
{
    if (currentMp3Process) {
        currentMp3Process->kill();
        delete currentMp3Process;
    }
}

bool ncmdumpGUIwithoutgo::initialize()
{
    if (dllLoaded) return true;
    loadDllFunctions();
    dllLoaded = true;
    QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
    if (!QFile::exists(ffmpegPath)) {
        appendLog("警告：未找到 ffmpeg.exe，MP3转换功能不可用");
    }
    return true;
}

void ncmdumpGUIwithoutgo::loadDllFunctions()
{
    lib.setFileName("libncmdump");
    if (!lib.load()) {
        appendLog("错误: 无法加载 libncmdump.dll");
        return;
    }
    createCrypt = (CreateCryptFunc)lib.resolve("CreateNeteaseCrypt");
    dumpFunc = (DumpFunc)lib.resolve("Dump");
    fixMetadataFunc = (FixMetadataFunc)lib.resolve("FixMetadata");
    destroyCrypt = (DestroyCryptFunc)lib.resolve("DestroyNeteaseCrypt");
    if (!createCrypt || !dumpFunc || !fixMetadataFunc || !destroyCrypt) {
        appendLog("错误: DLL 中缺少必要的导出函数");
    }
    else {
        appendLog("DLL 加载成功");
    }
}

void ncmdumpGUIwithoutgo::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择 NCM 文件", "", "NCM 文件 (*.ncm)");
    if (files.isEmpty()) return;
    for (const QString& file : files) {
        bool exists = false;
        for (int i = 0; i < fileListWidget->count(); ++i) {
            if (fileListWidget->item(i)->text() == file) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            QListWidgetItem* item = new QListWidgetItem(file, fileListWidget);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
        }
    }
    appendLog(QString("已添加 %1 个文件，当前共 %2 个").arg(files.size()).arg(fileListWidget->count()));
}

void ncmdumpGUIwithoutgo::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择包含 NCM 文件的文件夹");
    if (dir.isEmpty()) return;
    QDirIterator it(dir, QStringList() << "*.ncm", QDir::Files, QDirIterator::Subdirectories);
    int added = 0;
    while (it.hasNext()) {
        QString file = it.next();
        bool exists = false;
        for (int i = 0; i < fileListWidget->count(); ++i) {
            if (fileListWidget->item(i)->text() == file) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            QListWidgetItem* item = new QListWidgetItem(file, fileListWidget);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            added++;
        }
    }
    appendLog(QString("从文件夹添加了 %1 个 NCM 文件，当前共 %2 个").arg(added).arg(fileListWidget->count()));
}

void ncmdumpGUIwithoutgo::onClearFiles()
{
    fileListWidget->clear();
    appendLog("已清空文件列表");
}

void ncmdumpGUIwithoutgo::onDeleteSelected()
{
    QList<QListWidgetItem*> itemsToRemove;
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QListWidgetItem* item = fileListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            itemsToRemove.append(item);
        }
    }
    if (itemsToRemove.isEmpty()) return;
    for (QListWidgetItem* item : itemsToRemove) {
        delete fileListWidget->takeItem(fileListWidget->row(item));
    }
    appendLog(QString("已删除 %1 个文件，剩余 %2 个").arg(itemsToRemove.size()).arg(fileListWidget->count()));
}

void ncmdumpGUIwithoutgo::onSelectOutputDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择输出目录");
    if (!dir.isEmpty()) {
        outputDirEdit->setText(dir);
    }
}

QString ncmdumpGUIwithoutgo::convertFile(const QString& ncmPath, const QString& outputDir)
{
    if (!createCrypt) return QString();
    QByteArray utf8Path = ncmPath.toUtf8();
    const char* outParam = "";
    QByteArray utf8Out;
    if (!outputDir.isEmpty()) {
        utf8Out = outputDir.toUtf8();
        outParam = utf8Out.constData();
    }
    int result = SafeCallDll(utf8Path.constData(), outParam,
        createCrypt, dumpFunc, fixMetadataFunc, destroyCrypt);
    if (result != 0) return QString();
    QFileInfo info(ncmPath);
    QString baseName = info.completeBaseName();
    QString targetDir = outputDir.isEmpty() ? info.absolutePath() : outputDir;
    QString mp3Path = targetDir + "/" + baseName + ".mp3";
    QString flacPath = targetDir + "/" + baseName + ".flac";
    if (QFile::exists(mp3Path)) return mp3Path;
    if (QFile::exists(flacPath)) return flacPath;
    return QString();
}

bool ncmdumpGUIwithoutgo::convertToMp3Async(const QString& inputFlac, const QString& outputMp3)
{
    QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
    if (!QFile::exists(ffmpegPath)) {
        appendLog("错误: 找不到 ffmpeg.exe");
        return false;
    }
    if (currentMp3Process) return false;
    currentMp3Process = new QProcess(this);
    currentMp3Process->setProgram(ffmpegPath);
    QStringList args;
    args << "-i" << inputFlac << "-b:a" << "192k" << outputMp3;
    currentMp3Process->setArguments(args);
    connect(currentMp3Process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, &ncmdumpGUIwithoutgo::onMp3ConvertFinished);
    connect(currentMp3Process, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        appendLog(QString("FFmpeg 错误: %1").arg(error));
        if (currentMp3Process) {
            currentMp3Process->deleteLater();
            currentMp3Process = nullptr;
        }
        processNextFile();
        });
    currentMp3Process->start();
    if (!currentMp3Process->waitForStarted(3000)) {
        appendLog("FFmpeg 启动失败");
        currentMp3Process->deleteLater();
        currentMp3Process = nullptr;
        return false;
    }
    return true;
}

void ncmdumpGUIwithoutgo::onMp3ConvertFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (currentMp3Process) {
        bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
        if (success) {
            appendLog(QString("MP3转换成功: %1").arg(currentMp3File));
            if (!keepFlacAfterMp3) {
                if (QFile::remove(currentFlacFile))
                    appendLog("已删除临时FLAC: " + currentFlacFile);
            }
            else {
                appendLog("保留FLAC: " + currentFlacFile);
            }
        }
        else {
            appendLog(QString("MP3转换失败: %1 (退出码: %2)").arg(currentFlacFile).arg(exitCode));
        }
        currentMp3Process->deleteLater();
        currentMp3Process = nullptr;
        currentFlacFile.clear();
        currentMp3File.clear();
    }
    processNextFile();
}

void ncmdumpGUIwithoutgo::processNextFile()
{
    if (!flacQueue.isEmpty()) {
        QString flac = flacQueue.dequeue();
        QString ncm = ncmQueue.dequeue();
        QFileInfo info(flac);
        QString baseName = info.completeBaseName();
        QString targetDir = info.absolutePath();
        QString mp3Path = targetDir + "/" + baseName + ".mp3";
        currentFlacFile = flac;
        currentMp3File = mp3Path;
        appendLog(QString("正在转换为MP3: %1").arg(mp3Path));
        if (!convertToMp3Async(flac, mp3Path)) {
            appendLog("MP3转换启动失败，跳过: " + flac);
            processNextFile();
        }
    }
    else {
        appendLog("所有转换任务已完成。");
        progressBar->setRange(0, 100);
        progressBar->setValue(100);
        btnConvert->setEnabled(true);
        QMessageBox::information(this, "完成", "所有文件处理完毕。");
    }
}

void ncmdumpGUIwithoutgo::onConvert()
{
    if (fileListWidget->count() == 0) {
        QMessageBox::warning(this, "警告", "没有要转换的文件");
        return;
    }
    if (!createCrypt) {
        QMessageBox::critical(this, "错误", "DLL 未正确加载");
        return;
    }
    QString outputDir = outputDirEdit->text().trimmed();
    bool needMp3 = checkBoxToMp3->isChecked();
    bool keepFlac = checkBoxKeepFlac->isChecked();
    if (needMp3) {
        QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
        if (!QFile::exists(ffmpegPath)) {
            appendLog("警告: 未找到 ffmpeg.exe，将只解密为源格式。");
            needMp3 = false;
            keepFlac = false;
        }
    }
    btnConvert->setEnabled(false);
    flacQueue.clear();
    ncmQueue.clear();
    processedCount = 0;
    totalFiles = fileListWidget->count();
    keepFlacAfterMp3 = keepFlac;
    int successDecrypt = 0, failDecrypt = 0;
    QList<QString> flacList, ncmList;
    for (int i = 0; i < fileListWidget->count(); ++i) {
        QString file = fileListWidget->item(i)->text();
        appendLog(QString("正在解密: %1").arg(file));
        QString outputFilePath = convertFile(file, outputDir);
        if (outputFilePath.isEmpty()) {
            failDecrypt++;
            appendLog(QString("解密失败: %1").arg(file));
            continue;
        }
        QFileInfo outInfo(outputFilePath);
        QString format = outInfo.suffix().toUpper();
        if (needMp3 && format == "FLAC") {
            flacList.append(outputFilePath);
            ncmList.append(file);
            appendLog(QString("解密成功，将转换为MP3: %1").arg(outputFilePath));
        }
        else {
            appendLog(QString("解密成功: %1 (格式: %2)").arg(outputFilePath).arg(format));
        }
        successDecrypt++;
    }
    if (!flacList.isEmpty()) {
        for (const QString& f : flacList) flacQueue.enqueue(f);
        for (const QString& n : ncmList) ncmQueue.enqueue(n);
        progressBar->setRange(0, 0);
        progressBar->setValue(0);
        appendLog(QString("开始异步转换MP3，共 %1 个文件...").arg(flacQueue.size()));
        processNextFile();
    }
    else {
        appendLog(QString("解密完成，成功: %1，失败: %2").arg(successDecrypt).arg(failDecrypt));
        progressBar->setRange(0, 100);
        progressBar->setValue(100);
        btnConvert->setEnabled(true);
        QMessageBox::information(this, "完成", QString("解密成功: %1\n失败: %2").arg(successDecrypt).arg(failDecrypt));
    }
}

void ncmdumpGUIwithoutgo::appendLog(const QString& msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    logEdit->append(QString("[%1] %2").arg(timestamp, msg));
}

void ncmdumpGUIwithoutgo::onSettings()
{
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("设置");
    settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    settingsDialog.setFixedSize(600, 400);
    QVBoxLayout* layout = new QVBoxLayout(&settingsDialog);
    QLabel* label = new QLabel("此页开发中...", &settingsDialog);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    settingsDialog.exec();
}

void ncmdumpGUIwithoutgo::onAbout()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("关于 “NCM Converter”");
    aboutDialog.setWindowFlags(aboutDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    aboutDialog.setFixedSize(520, 700);

    QVBoxLayout* mainLayout = new QVBoxLayout(&aboutDialog);
    mainLayout->setSpacing(8);

    // ---- 左上角：图标 + 名称（水平排列） ----
    QHBoxLayout* titleLayout = new QHBoxLayout;
    titleLayout->setAlignment(Qt::AlignLeft);

    QLabel* iconLabel = new QLabel(&aboutDialog);
    QIcon icon(":/icons/NCMC.ico");
    if (!icon.isNull())
        iconLabel->setPixmap(icon.pixmap(64, 64));
    else
        iconLabel->setText("[图标]");
    titleLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("NCM Converter", &aboutDialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);

    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    // ---- 版本、版权、作者等 ----
    QLabel* versionLabel = new QLabel("1.0 - Release", &aboutDialog);
    mainLayout->addWidget(versionLabel);

    QLabel* copyrightLabel = new QLabel("Copyright (C) 2026 ZHB3306", &aboutDialog);
    mainLayout->addWidget(copyrightLabel);

    QLabel* authorLabel = new QLabel("作者: ZHB3306", &aboutDialog);
    mainLayout->addWidget(authorLabel);

    QLabel* licenseLabel = new QLabel("本软件遵循 GPL-3.0 协议", &aboutDialog);
    mainLayout->addWidget(licenseLabel);

    QLabel* linkLabel = new QLabel(
        "<a href='https://github.com/zhb3306/ncmdump-GUI' style='color: #6b5b9b;'>GitHub 项目主页</a>",
        &aboutDialog
    );
    linkLabel->setOpenExternalLinks(true);
    mainLayout->addWidget(linkLabel);

    QLabel* warrantyLabel = new QLabel("本软件按“现状”提供，不提供任何担保。", &aboutDialog);
    mainLayout->addWidget(warrantyLabel);

    // ---- 引用信息（可点击链接） ----
    QLabel* refTitleLabel = new QLabel("引用项目：", &aboutDialog);
    QFont refFont = refTitleLabel->font();
    refFont.setBold(true);
    refTitleLabel->setFont(refFont);
    mainLayout->addWidget(refTitleLabel);

    QLabel* refContentLabel = new QLabel(
        "本软件使用了以下开源项目：<br>"
        "• <a href='https://github.com/taurusxin/ncmdump' style='color: #6b5b9b;'>libncmdump</a>（MIT）<br>"
        "• <a href='https://ffmpeg.org/' style='color: #6b5b9b;'>FFmpeg</a>（LGPL-2.1+）<br>"
        "• <a href='https://www.qt.io/' style='color: #6b5b9b;'>Qt 框架</a>（LGPL-3.0 / GPL-3.0）<br>"
        "感谢他们的付出！",
        &aboutDialog
    );
    refContentLabel->setWordWrap(true);
    refContentLabel->setOpenExternalLinks(true);
    refContentLabel->setStyleSheet("color: #3a4c6c; font-size: 10pt;");
    mainLayout->addWidget(refContentLabel);

    // ---- GPL-3.0 协议全文 ----
    QTextEdit* textEdit = new QTextEdit(&aboutDialog);
    textEdit->setReadOnly(true);
    QFile file(":/gpl-3.0.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        textEdit->setPlainText(stream.readAll());
        file.close();
    }
    else {
        textEdit->setPlainText("GPL-3.0 协议文本未找到，请检查资源文件。");
    }
    textEdit->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(textEdit);

    aboutDialog.exec();
}
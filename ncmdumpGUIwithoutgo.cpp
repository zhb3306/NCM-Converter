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
#include <QToolButton>
#include <QCheckBox>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTabWidget>
#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QDialog>
#include <QTextEdit>
#include <QStyle>
#include <QPixmap>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QRadioButton>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
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
    : QMainWindow(parent), currentMp3Process(nullptr), totalFiles(0), processedCount(0), keepFlacAfterMp3(false), dllLoaded(false), flashTimer(nullptr), flashRed(false), networkManager(nullptr)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle("NCM Converter");

    setAcceptDrops(true);

    QMenuBar* menuBar = new QMenuBar(this);
    mainMenu = new QMenu("NCM Converter", this);
    actionSettings = new QAction("设置", this);
    actionAbout = new QAction("关于", this);
    mainMenu->addAction(actionSettings);
    mainMenu->addAction(actionAbout);

    logAction = new QAction("日志", this);
    logAction->setVisible(false);
    mainMenu->addAction(logAction);

    menuBar->addMenu(mainMenu);
    setMenuBar(menuBar);

    QFont mbFont = menuBar->font();
    mbFont.setBold(true);
    mbFont.setPointSize(mbFont.pointSize() + 1);
    menuBar->setFont(mbFont);

    connect(actionSettings, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onSettings);
    connect(actionAbout, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onAbout);
    connect(logAction, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onShowLog);

    updateMenu = new QMenu("更新", this);
    actionCheckUpdate = new QAction("检查更新", this);
    actionVersionInfo = new QAction("当前版本 " NCM_CONVERTER_VERSION, this);
    actionVersionInfo->setEnabled(false);
    updateMenu->addAction(actionCheckUpdate);
    updateMenu->addSeparator();
    updateMenu->addAction(actionVersionInfo);

    connect(actionCheckUpdate, &QAction::triggered, this, &ncmdumpGUIwithoutgo::onCheckUpdate);

    updateBtn = new QPushButton("更新", this);
    updateBtn->setFlat(true);
    updateBtn->setCursor(Qt::PointingHandCursor);
    QObject::connect(updateBtn, &QPushButton::clicked, [this]() {
        updateMenu->exec(updateBtn->mapToGlobal(QPoint(0, updateBtn->height())));
    });

    menuBar->setCornerWidget(updateBtn);

    // 根据设置决定日志菜单是否可见
    {
        QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
        QSettings settings(iniPath, QSettings::IniFormat);
        logAction->setVisible(settings.value("DebugMode", false).toBool());
    }

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

   /*
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
        "QMenuBar { background: transparent; color: #2b2f4c; border: none; }"
        "QMenuBar::item:selected { background: rgba(255,255,255,0.3); border-radius: 10px; }"
        "QMenu { background: rgba(255,255,255,0.85); border: none; outline: none; border-radius: 0px; margin: 0px; padding: 5px; }"
        "QMenu::item { background: transparent; padding: 6px 20px; }"
        "QMenu::item:selected { background: rgba(159,139,203,0.3); }"
        "QDialog { background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "   stop:0 #c0d9ff, stop:0.5 #e0c8ff, stop:1 #ffe6c7); }"
        "QDialog QLabel { color: #2b2f4c; }"
        "QDialog QPushButton {"
        "   background: rgba(255,255,255,0.6);"
        "   border: none;"
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
    */

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
    addFiles(files);
}

void ncmdumpGUIwithoutgo::addFiles(const QStringList& files)
{
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
    QStringList fileList;
    QDirIterator it(dir, QStringList() << "*.ncm", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        fileList << it.next();
    }
    if (!fileList.isEmpty()) {
        addFiles(fileList);
    }
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

void ncmdumpGUIwithoutgo::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ncmdumpGUIwithoutgo::dropEvent(QDropEvent* event)
{
    const QMimeData* mime = event->mimeData();
    if (!mime->hasUrls()) return;

    QStringList fileList;
    for (const QUrl& url : mime->urls()) {
        QString path = url.toLocalFile();
        if (!path.isEmpty()) {
            QFileInfo info(path);
            if (info.isDir()) {
                QDirIterator it(path, QStringList() << "*.ncm", QDir::Files, QDirIterator::Subdirectories);
                while (it.hasNext()) {
                    fileList << it.next();
                }
            }
            else if (info.suffix().toLower() == "ncm") {
                fileList << path;
            }
        }
    }
    if (!fileList.isEmpty()) {
        addFiles(fileList);
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

        QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
        QSettings settings(iniPath, QSettings::IniFormat);
        if (settings.value("DebugMode", false).toBool()) {
            QString statusStr = (exitStatus == QProcess::NormalExit) ? "正常退出" : "异常退出";
            appendLog(QString("退出码：[%1][%2]").arg(exitCode).arg(statusStr));
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
    settingsDialog.setFixedSize(400, 320);

    QVBoxLayout* mainLayout = new QVBoxLayout(&settingsDialog);

    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    QString dnc = settings.value("DNC", "YES").toString();
    bool debugMode = settings.value("DebugMode", false).toBool();
    bool disableUpdate = settings.value("DisableUpdate", false).toBool();
    bool disableFlash = settings.value("DisableFlash", false).toBool();

    QTabWidget* tabWidget = new QTabWidget(&settingsDialog);

    QWidget* convertTab = new QWidget;
    QVBoxLayout* convertLayout = new QVBoxLayout(convertTab);

    QLabel* label = new QLabel("双击 .ncm 文件后：", convertTab);
    convertLayout->addWidget(label);

    QRadioButton* radioConvert = new QRadioButton("转换（打开 NCM Converter）", convertTab);
    QRadioButton* radioPlay = new QRadioButton("播放（打开网易云音乐）", convertTab);
    if (dnc == "YES") {
        radioConvert->setChecked(true);
    }
    else {
        radioPlay->setChecked(true);
    }
    convertLayout->addWidget(radioConvert);
    convertLayout->addWidget(radioPlay);
    convertLayout->addStretch();

    QWidget* devTab = new QWidget;
    QVBoxLayout* devLayout = new QVBoxLayout(devTab);

    QHBoxLayout* debugLayout = new QHBoxLayout;
    QCheckBox* debugCheck = new QCheckBox("Debug模式", devTab);
    debugCheck->setChecked(debugMode);
    debugLayout->addWidget(debugCheck);

    QToolButton* debugHelpBtn = new QToolButton(devTab);
    debugHelpBtn->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    debugHelpBtn->setToolTip("开启后，每次转换完成都会在日志中显示退出码。");
    QObject::connect(debugHelpBtn, &QToolButton::clicked, [&]() {
        QMessageBox::information(&settingsDialog, "Debug模式",
            "开启 Debug 模式后，每个文件转换完成时，日志末尾会追加该次 FFmpeg 进程的退出码与状态，便于排查转换失败原因。");
    });
    debugLayout->addWidget(debugHelpBtn);
    debugLayout->addStretch();
    devLayout->addLayout(debugLayout);
    devLayout->addStretch();

    QWidget* updateTab = new QWidget;
    QVBoxLayout* updateLayout = new QVBoxLayout(updateTab);

    QCheckBox* disableUpdateCheck = new QCheckBox("禁用启动时检查更新", updateTab);
    disableUpdateCheck->setChecked(disableUpdate);
    updateLayout->addWidget(disableUpdateCheck);

    QLabel* updateHint1 = new QLabel("勾选后将不会在启动时检查更新，手动检查时仍可正常使用。", updateTab);
    updateHint1->setWordWrap(true);
    updateHint1->setStyleSheet("color: gray; font-size: 11px;");
    updateLayout->addWidget(updateHint1);

    updateLayout->addSpacing(8);

    QCheckBox* disableFlashCheck = new QCheckBox("禁用更新闪烁", updateTab);
    disableFlashCheck->setChecked(disableFlash);
    updateLayout->addWidget(disableFlashCheck);

    QLabel* updateHint2 = new QLabel("勾选后，有更新可用时更新按钮将只显示文字，不会红黄闪烁。", updateTab);
    updateHint2->setWordWrap(true);
    updateHint2->setStyleSheet("color: gray; font-size: 11px;");
    updateLayout->addWidget(updateHint2);

    updateLayout->addStretch();

    tabWidget->addTab(convertTab, "转换相关");
    tabWidget->addTab(updateTab, "更新相关");
    tabWidget->addTab(devTab, "开发");
    mainLayout->addWidget(tabWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* saveBtn = new QPushButton("保存设置", &settingsDialog);
    QPushButton* closeBtn = new QPushButton("关闭", &settingsDialog);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    QObject::connect(saveBtn, &QPushButton::clicked, [&]() {
        if (radioConvert->isChecked()) {
            settings.setValue("DNC", "YES");
        }
        else {
            settings.setValue("DNC", "NO");
        }
        bool dbg = debugCheck->isChecked();
        settings.setValue("DebugMode", dbg);
        settings.setValue("DisableUpdate", disableUpdateCheck->isChecked());
        settings.setValue("DisableFlash", disableFlashCheck->isChecked());
        settings.sync();
        logAction->setVisible(dbg);
        appendLog("设置已保存");
        QMessageBox::information(&settingsDialog, "成功", "设置已保存。");
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &settingsDialog, &QDialog::accept);

    settingsDialog.exec();
}

void ncmdumpGUIwithoutgo::onAbout()
{
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle("关于 “NCM Converter”");
    aboutDialog.setWindowFlags(aboutDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    aboutDialog.setFixedSize(620, 520);

    QVBoxLayout* mainLayout = new QVBoxLayout(&aboutDialog);
    mainLayout->setSpacing(8);

    QTabWidget* tabWidget = new QTabWidget(&aboutDialog);

    // ========== 标签页1：基本 ==========
    QWidget* basicTab = new QWidget(tabWidget);
    QVBoxLayout* basicLayout = new QVBoxLayout(basicTab);
    basicLayout->setSpacing(8);

    // 图标 + 名称
    QHBoxLayout* titleLayout = new QHBoxLayout;
    titleLayout->setAlignment(Qt::AlignLeft);
    QLabel* iconLabel = new QLabel(basicTab);
    QIcon icon(":/icons/NCMC.ico");
    if (!icon.isNull())
        iconLabel->setPixmap(icon.pixmap(64, 64));
    else
        iconLabel->setText("[图标]");
    titleLayout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("NCM Converter", basicTab);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    basicLayout->addLayout(titleLayout);

    QLabel* versionLabel = new QLabel("版本 " NCM_CONVERTER_VERSION " - Release", basicTab);
    basicLayout->addWidget(versionLabel);

    QLabel* copyrightLabel = new QLabel("Copyright (C) 2026 ZHB3306", basicTab);
    basicLayout->addWidget(copyrightLabel);

    QLabel* authorLabel = new QLabel("作者: ZHB3306", basicTab);
    basicLayout->addWidget(authorLabel);

    QLabel* licenseLabel = new QLabel("本软件遵循 GPL-3.0 协议", basicTab);
    basicLayout->addWidget(licenseLabel);

    QLabel* linkLabel = new QLabel(
        "<a href='https://github.com/zhb3306/NCM-Converter' style='color: #6b5b9b;'>GitHub 项目主页</a>",
        basicTab);
    linkLabel->setOpenExternalLinks(true);
    basicLayout->addWidget(linkLabel);

    QLabel* warrantyLabel = new QLabel("本软件按“现状”提供，不提供任何担保。", basicTab);
    basicLayout->addWidget(warrantyLabel);

    QLabel* refTitleLabel = new QLabel("引用", basicTab);
    QFont refFont = refTitleLabel->font();
    refFont.setBold(true);
    refTitleLabel->setFont(refFont);
    basicLayout->addWidget(refTitleLabel);

    QLabel* refContentLabel = new QLabel(
        QStringLiteral(
            "本软件使用了以下开源项目：<br>"
            "• <a href='https://github.com/taurusxin/ncmdump' style='color: #6b5b9b;'>libncmdump</a>（MIT）<br>"
            "• <a href='https://ffmpeg.org/' style='color: #6b5b9b;'>FFmpeg</a>（LGPL-2.1+）<br>"
            "• <a href='https://www.qt.io/' style='color: #6b5b9b;'>Qt 框架</a>（LGPL-3.0 / GPL-3.0）"
        ),
        basicTab);
    refContentLabel->setWordWrap(true);
    refContentLabel->setOpenExternalLinks(true);
    refContentLabel->setStyleSheet("color: #3a4c6c; font-size: 10pt;");
    basicLayout->addWidget(refContentLabel);

    basicLayout->addStretch();
    tabWidget->addTab(basicTab, "基本");

    // ========== 标签页2：版本信息 ==========
    QWidget* versionTab = new QWidget(tabWidget);
    QVBoxLayout* versionLayout = new QVBoxLayout(versionTab);
    versionLayout->setSpacing(10);

    QLabel* versionInfoTitle = new QLabel("版本信息", versionTab);
    QFont vTitleFont = versionInfoTitle->font();
    vTitleFont.setPointSize(12);
    vTitleFont.setBold(true);
    versionInfoTitle->setFont(vTitleFont);
    versionLayout->addWidget(versionInfoTitle);

    // 详细信息
    QLabel* versionDetails = new QLabel(
        QString("软件版本：" NCM_CONVERTER_VERSION " - Release\n")
        + "Qt 版本：" + QT_VERSION_STR + "\n"
        + "构建日期：" + __DATE__ + " " + __TIME__ + "\n"
        + "编译器：MSVC 2019 (64-bit)\n"
        + "操作系统：Windows 7 / 10 / 11 (64-bit)",
        versionTab);
    versionDetails->setStyleSheet("color: #2b2f4c; font-size: 11pt;");
    versionDetails->setWordWrap(true);
    versionLayout->addWidget(versionDetails);

    QLabel* extraInfo = new QLabel(
        "本版本基于 Qt 5.15.2 开发，使用 Visual Studio 2019 工具链编译。\n"
        "更多信息请访问项目主页。",
        versionTab);
    extraInfo->setStyleSheet("color: #5a6c8c; font-size: 10pt;");
    extraInfo->setWordWrap(true);
    versionLayout->addWidget(extraInfo);

    // 🆕 版本说明
    QLabel* releaseNoteTitle = new QLabel("版本说明", versionTab);
    QFont rnFont = releaseNoteTitle->font();
    rnFont.setBold(true);
    releaseNoteTitle->setFont(rnFont);
    versionLayout->addWidget(releaseNoteTitle);

    QTextEdit* releaseNoteEdit = new QTextEdit(versionTab);
    releaseNoteEdit->setReadOnly(true);
    releaseNoteEdit->setPlainText("1.本次更新\n"
                                  "(1)因为测出了关于样式表的BUG，因此暂时移除了样式表\n"
                                  "(2)加入了版本信息与说明\n"
                                  "2.未来更新\n"
                                  "(1)添加“更新”部分，可以检查更新，可以在菜单栏显示（将不会出现“强制更新制”）。\n"
                                  "3.声明：\n"
                                  "(1)除非新系统不支持，否则将永远不会更新Qt的版本。（为了兼容Windows7）");
    releaseNoteEdit->setMaximumHeight(120);
    releaseNoteEdit->setStyleSheet(
        "background: rgba(255,255,255,0.4);"
        "border: 1px solid rgba(0,0,0,0.1);"
        "border-radius: 8px;"
        "padding: 6px;"
        "color: #2b2f4c;"
        "font-size: 10pt;"
    );
    versionLayout->addWidget(releaseNoteEdit);

    versionLayout->addStretch();
    tabWidget->addTab(versionTab, "版本信息");

    // ---- 将 TabWidget 添加到主布局 ----
    mainLayout->addWidget(tabWidget);

    // ---- 公共底部：GPL-3.0 协议 ----
    QTextEdit* textEdit = new QTextEdit(&aboutDialog);
    textEdit->setReadOnly(true);
    QFile file(":/gpl-3.0.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        textEdit->setPlainText(stream.readAll());
        file.close();
    }
    else {
        textEdit->setPlainText("＞︿＜ GPL-3.0 协议文本未找到，请检查资源文件。");
    }
    textEdit->setAlignment(Qt::AlignCenter);
    textEdit->setMaximumHeight(150);
    mainLayout->addWidget(textEdit);

    aboutDialog.exec();
}
void ncmdumpGUIwithoutgo::associateFileType()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString exePath = appPath + "/NCM Converter.exe";
    QString iconPath = appPath + "/icons/NCM_File.ico";

    if (!QFile::exists(iconPath)) {
        appendLog("＞︿＜ 警告：图标文件不存在，无法设置文件图标。");
        iconPath = exePath;
    }

    QSettings extSettings("HKEY_CURRENT_USER\\Software\\Classes\\.ncm", QSettings::NativeFormat);
    extSettings.setValue("Default", "NCMConverter.ncm");

    QSettings progSettings("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm", QSettings::NativeFormat);
    progSettings.setValue("Default", "NCM 加密音乐文件");

    QSettings iconSettings("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm\\DefaultIcon", QSettings::NativeFormat);
    iconSettings.setValue("Default", iconPath + ",0");

    QSettings cmdSettings("HKEY_CURRENT_USER\\Software\\Classes\\NCMConverter.ncm\\shell\\open\\command", QSettings::NativeFormat);
    cmdSettings.setValue("Default", "\"" + exePath + "\" \"%1\"");
}

void ncmdumpGUIwithoutgo::onShowLog()
{
    QDialog logDialog(this);
    logDialog.setWindowTitle("运行日志");
    logDialog.setWindowFlags(logDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    logDialog.resize(680, 480);

    QVBoxLayout* mainLayout = new QVBoxLayout(&logDialog);

    QTextEdit* logView = new QTextEdit(&logDialog);
    logView->setReadOnly(true);
    logView->setPlainText(logEdit->toPlainText());
    logView->moveCursor(QTextCursor::End);
    mainLayout->addWidget(logView);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* exportBtn = new QPushButton("导出日志", &logDialog);
    QPushButton* closeBtn = new QPushButton("关闭", &logDialog);
    btnLayout->addStretch();
    btnLayout->addWidget(exportBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    QObject::connect(exportBtn, &QPushButton::clicked, [&]() {
        QString savePath = QFileDialog::getSaveFileName(&logDialog, "导出日志", "NCMConverter_log.txt", "文本文件 (*.txt)");
        if (!savePath.isEmpty()) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << logEdit->toPlainText();
                file.close();
                QMessageBox::information(&logDialog, "成功", "日志已导出。");
            }
            else {
                QMessageBox::warning(&logDialog, "错误", "无法写入文件。");
            }
        }
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &logDialog, &QDialog::accept);

    logDialog.exec();
}

static QList<int> parseVersion(const QString& v)
{
    QList<int> parts;
    for (const QString& s : v.split('.'))
        parts.append(s.toInt());
    return parts;
}

static bool isNewer(const QString& latest, const QString& current)
{
    QList<int> l = parseVersion(latest);
    QList<int> c = parseVersion(current);
    int len = qMax(l.size(), c.size());
    while (l.size() < len) l.append(0);
    while (c.size() < len) c.append(0);
    for (int i = 0; i < len; ++i) {
        if (l[i] > c[i]) return true;
        if (l[i] < c[i]) return false;
    }
    return false;
}

void ncmdumpGUIwithoutgo::onCheckUpdate()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("更新");
    dlg->setWindowFlags(dlg->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg->setFixedSize(420, 520);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* layout = new QVBoxLayout(dlg);
    layout->setSpacing(8);
    layout->setContentsMargins(40, 24, 40, 24);

    QLabel* iconLabel = new QLabel(dlg);
    QIcon icon(":/icons/NCMC.ico");
    iconLabel->setPixmap(icon.pixmap(120, 120));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel("NCM Converter", dlg);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel* versionLabel = new QLabel("当前版本: " NCM_CONVERTER_VERSION, dlg);
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    QLabel* infoLabel = new QLabel(dlg);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->hide();
    layout->addWidget(infoLabel);

    layout->addSpacing(8);

    QPushButton* downloadBtn = new QPushButton("下载更新", dlg);
    downloadBtn->setMinimumWidth(140);
    downloadBtn->hide();
    downloadBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; border-radius: 4px; padding: 8px 16px; } QPushButton:hover { background-color: #45a049; }");
    layout->addWidget(downloadBtn, 0, Qt::AlignCenter);
    layout->addSpacing(4);

    QPushButton* checkBtn = new QPushButton("检查更新", dlg);
    checkBtn->setMinimumWidth(140);
    layout->addWidget(checkBtn, 0, Qt::AlignCenter);
    layout->addSpacing(4);

    QProgressBar* progBar = new QProgressBar(dlg);
    progBar->setRange(0, 100);
    progBar->setValue(0);
    progBar->setTextVisible(false);
    layout->addWidget(progBar);

    layout->addStretch();
    layout->addStretch();
    layout->addStretch();
    layout->addStretch();

    connect(checkBtn, &QPushButton::clicked, dlg, [this, dlg, versionLabel, infoLabel, checkBtn, downloadBtn, progBar]() {
        infoLabel->hide();
        infoLabel->setText("");
        progBar->setRange(0, 0);  // 不确定模式
        progBar->setValue(0);
        checkBtn->setEnabled(false);

        QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
        QSettings settings(iniPath, QSettings::IniFormat);
        QString wayUrl = settings.value("WayURL", "https://cdn.zhb1323306.qzz.io/way_ncmconverter.json").toString();

        appendLog("正在获取更新服务器地址...");

        if (!networkManager) {
            networkManager = new QNetworkAccessManager(this);
            // Win7 上禁用代理检测，避免 WinHTTP WPAD 卡 20 秒
            if (QSysInfo::productVersion() == QStringLiteral("7"))
                networkManager->setProxy(QNetworkProxy::NoProxy);
        }
        QNetworkRequest req{ QUrl(wayUrl) };
        req.setHeader(QNetworkRequest::UserAgentHeader, "NCMC/" NCM_CONVERTER_VERSION);
        req.setTransferTimeout(10000);
        QNetworkReply* wayReply = networkManager->get(req);

        connect(wayReply, &QNetworkReply::finished, dlg, [this, dlg, wayReply, versionLabel, infoLabel, checkBtn, downloadBtn, progBar, iniPath]() {
            wayReply->deleteLater();

            QString fullUrl;

            bool gotFromWay = false;
            if (wayReply->error() == QNetworkReply::NoError) {
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(wayReply->readAll(), &err);
                if (err.error == QJsonParseError::NoError) {
                    QString updateServer = doc.object()["update_server"].toString();
                    if (!updateServer.isEmpty()) {
                        if (!updateServer.startsWith("http"))
                            updateServer = "https://" + updateServer;
                        fullUrl = updateServer + "/update.json";
                        gotFromWay = true;
                        appendLog("更新服务器: " + updateServer);
                    }
                }
            }

            if (!gotFromWay) {
                QSettings settings(iniPath, QSettings::IniFormat);
                fullUrl = settings.value("UpdateURL", "https://cdn.zhb1323306.qzz.io/NCM-Converter/update.json").toString();
                if (wayReply->error() != QNetworkReply::NoError)
                    appendLog("无法获取更新服务器地址，使用本地默认地址");
            }

            appendLog("正在检查更新...");
            QNetworkRequest req2{ QUrl(fullUrl) };
            req2.setHeader(QNetworkRequest::UserAgentHeader, "NCMC/" NCM_CONVERTER_VERSION);
            req2.setTransferTimeout(10000);
            QNetworkReply* reply = networkManager->get(req2);

            connect(reply, &QNetworkReply::finished, dlg, [this, dlg, reply, versionLabel, infoLabel, checkBtn, downloadBtn, progBar]() {
                reply->deleteLater();
                progBar->setRange(0, 100);
                progBar->setValue(100);
                checkBtn->setEnabled(true);

                if (reply->error() != QNetworkReply::NoError) {
                    QString reason = reply->errorString();
                    appendLog("检查更新失败: " + reason);
                    infoLabel->setText(QString("检查更新失败了呢\n因为\n%1").arg(reason));
                    infoLabel->show();
                    return;
                }

                QByteArray data = reply->readAll();
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(data, &err);
                if (err.error != QJsonParseError::NoError) {
                    appendLog("更新信息解析失败: " + err.errorString());
                    infoLabel->setText(QString("检查更新失败了呢\n因为\n%1").arg(err.errorString()));
                    infoLabel->show();
                    return;
                }

                QJsonObject obj = doc.object();
                QString latestVersion  = obj["latest_version"].toString();
                QString downloadUrl    = obj["download_url"].toString();
                QString releaseDate    = obj["release_date"].toString();

                if (latestVersion.isEmpty()) {
                    infoLabel->setText("检查更新失败了呢\n因为\n未获取到版本信息");
                    infoLabel->show();
                    return;
                }

                appendLog(QString("服务器版本: %1，当前版本: " NCM_CONVERTER_VERSION).arg(latestVersion));

                if (isNewer(latestVersion, NCM_CONVERTER_VERSION)) {
                    updateDownloadUrl = downloadUrl;
                    latestUpdateVersion = latestVersion;
                    actionVersionInfo->setText(QString("当前版本 " NCM_CONVERTER_VERSION "\n最新版本 %1").arg(latestVersion));

                    infoLabel->setText(QString("更新版本: %1\n\n有更新可用了!\n\n发布日期: %2").arg(latestVersion, releaseDate));
                    infoLabel->show();

                    downloadBtn->show();
                    downloadBtn->disconnect();
                    connect(downloadBtn, &QPushButton::clicked, dlg, [this]() {
                        if (!updateDownloadUrl.isEmpty())
                            QDesktopServices::openUrl(QUrl(updateDownloadUrl));
                    });

                    startFlashTimer();
                }
                else {
                    infoLabel->setText("已经是最新版本了!");
                    infoLabel->show();
                    updateDownloadUrl.clear();
                    downloadBtn->hide();
                    stopFlashTimer();
                }
            });
        });
    });

    dlg->exec();
}

void ncmdumpGUIwithoutgo::doSilentCheckUpdate()
{
    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);

    if (settings.value("DisableUpdate", false).toBool()) {
        return;
    }

    QString wayUrl = settings.value("WayURL", "https://cdn.zhb1323306.qzz.io/way_ncmconverter.json").toString();

    if (!networkManager) {
        networkManager = new QNetworkAccessManager(this);
        // Win7 上禁用代理检测，避免 WinHTTP WPAD 卡 20 秒
        if (QSysInfo::productVersion() == QStringLiteral("7"))
            networkManager->setProxy(QNetworkProxy::NoProxy);
    }
    QNetworkRequest req{ QUrl(wayUrl) };
    req.setHeader(QNetworkRequest::UserAgentHeader, "NCMC/" NCM_CONVERTER_VERSION);
    req.setTransferTimeout(10000);
    QNetworkReply* wayReply = networkManager->get(req);

    connect(wayReply, &QNetworkReply::finished, this, [this, wayReply, iniPath]() {
        wayReply->deleteLater();

        QString fullUrl;

        bool gotFromWay = false;
        if (wayReply->error() == QNetworkReply::NoError) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(wayReply->readAll(), &err);
            if (err.error == QJsonParseError::NoError) {
                QString updateServer = doc.object()["update_server"].toString();
                if (!updateServer.isEmpty()) {
                    if (!updateServer.startsWith("http"))
                        updateServer = "https://" + updateServer;
                    fullUrl = updateServer + "/update.json";
                    gotFromWay = true;
                }
            }
        }

        if (!gotFromWay) {
            QSettings settings(iniPath, QSettings::IniFormat);
            fullUrl = settings.value("UpdateURL", "https://cdn.zhb1323306.qzz.io/NCM-Converter/update.json").toString();
        }

        if (fullUrl.isEmpty()) return;

        QNetworkRequest req2{ QUrl(fullUrl) };
        req2.setHeader(QNetworkRequest::UserAgentHeader, "NCMC/" NCM_CONVERTER_VERSION);
        req2.setTransferTimeout(10000);
        QNetworkReply* reply = networkManager->get(req2);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                return;  // 静默模式，不提示错误
            }

            QByteArray data = reply->readAll();
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(data, &err);
            if (err.error != QJsonParseError::NoError) {
                return;
            }

            QJsonObject obj = doc.object();
            QString latestVersion = obj["latest_version"].toString();
            QString downloadUrl   = obj["download_url"].toString();

            if (latestVersion.isEmpty()) return;

            if (isNewer(latestVersion, NCM_CONVERTER_VERSION)) {
                updateDownloadUrl = downloadUrl;
                latestUpdateVersion = latestVersion;
                actionVersionInfo->setText(QString("当前版本 " NCM_CONVERTER_VERSION "\n最新版本 %1").arg(latestVersion));
                startFlashTimer();
            }
        });
    });
}

void ncmdumpGUIwithoutgo::startFlashTimer()
{
    updateBtn->setText("有更新可用");

    QString iniPath = QCoreApplication::applicationDirPath() + "/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    if (settings.value("DisableFlash", false).toBool()) {
        return;  // 用户禁用了闪烁
    }

    if (!flashTimer) {
        flashTimer = new QTimer(this);
        connect(flashTimer, &QTimer::timeout, this, &ncmdumpGUIwithoutgo::onFlashTimerTick);
    }
    flashRed = true;
    flashTimer->start(1000);  // 每秒切换一次颜色
}

void ncmdumpGUIwithoutgo::stopFlashTimer()
{
    if (flashTimer) {
        flashTimer->stop();
    }
    updateBtn->setText("更新");
    updateBtn->setStyleSheet("");
}

void ncmdumpGUIwithoutgo::onFlashTimerTick()
{
    flashRed = !flashRed;
    if (flashRed) {
        updateBtn->setStyleSheet("color: red; font-weight: bold;");
    }
    else {
        updateBtn->setStyleSheet("color: #FFD700; font-weight: bold;");
    }
}
#pragma once

#include <QtWidgets/QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QLineEdit>
#include <QLibrary>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QQueue>

typedef void* (__cdecl* CreateCryptFunc)(const char*);
typedef int(__cdecl* DumpFunc)(void*, const char*);
typedef void(__cdecl* FixMetadataFunc)(void*);
typedef void(__cdecl* DestroyCryptFunc)(void*);

class ncmdumpGUIwithoutgo : public QMainWindow
{
    Q_OBJECT

public:
    ncmdumpGUIwithoutgo(QWidget* parent = nullptr);
    ~ncmdumpGUIwithoutgo();
    bool initialize();  

private slots:
    void onAddFiles();
    void onAddFolder();
    void onClearFiles();
    void onDeleteSelected();
    void onSelectOutputDir();
    void onConvert();
    void onSettings();
    void onAbout();
    void onMp3ConvertFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void appendLog(const QString& msg);
    QString convertFile(const QString& ncmPath, const QString& outputDir);
    bool convertToMp3Async(const QString& inputFlac, const QString& outputMp3);
    void loadDllFunctions();
    void processNextFile();

    QListWidget* fileListWidget;
    QPushButton* btnAddFiles, * btnAddFolder, * btnClear, * btnDelete, * btnSelectOutput, * btnConvert;
    QCheckBox* checkBoxToMp3;
    QCheckBox* checkBoxKeepFlac;
    QLineEdit* outputDirEdit;
    QTextEdit* logEdit;
    QProgressBar* progressBar;

    QMenu* mainMenu;
    QAction* actionSettings;
    QAction* actionAbout;

    QLibrary lib;
    CreateCryptFunc   createCrypt = nullptr;
    DumpFunc          dumpFunc = nullptr;
    FixMetadataFunc   fixMetadataFunc = nullptr;
    DestroyCryptFunc  destroyCrypt = nullptr;

    QQueue<QString> flacQueue;
    QQueue<QString> ncmQueue;
    QProcess* currentMp3Process;
    QString currentFlacFile;
    QString currentMp3File;
    int totalFiles;
    int processedCount;
    bool keepFlacAfterMp3;
    bool dllLoaded;         
};
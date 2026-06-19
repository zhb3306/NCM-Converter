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

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("NCM Converter");
    a.setOrganizationName("NCM Converter");
    a.setWindowIcon(QIcon(":/icons/NCMC.ico"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    QString fontPath = QCoreApplication::applicationDirPath() + "/fonts/SourceHanSansSC-Bold-2.otf";
    QFont font;
    if (QFile::exists(fontPath)) {
        int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty()) {
                font.setFamily(families.first());
                font.setWeight(QFont::Bold);
                qDebug() << "思源黑体加粗加载成功";
            }
        }
    }
 
    font.setPointSize(10);
    a.setFont(font);
    if (!font.family().isEmpty()) {
        a.setStyleSheet(QString("* { font-family: \"%1\"; font-weight: bold; }").arg(font.family()));
    }
    else {
        a.setStyleSheet("* { font-weight: bold; }");
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
    }

   
    SplashScreen splash;
    splash.show();

    QElapsedTimer timer;
    timer.start();

    ncmdumpGUIwithoutgo w;
    w.initialize();  

    int elapsed = timer.elapsed();
    int remaining = 3000 - elapsed;
    if (remaining < 0) remaining = 0;

    QTimer::singleShot(remaining, [&]() {
        splash.close();
        w.show();
        });

    return a.exec();
}
#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>

class SplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);

    void setStatus(const QString& status);

protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif
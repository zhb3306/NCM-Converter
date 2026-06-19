#include "splashscreen.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QPainter>
#include <QPainterPath>

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);  
    setFixedSize(450, 280); 

    QWidget* cardWidget = new QWidget(this);
    cardWidget->setObjectName("cardWidget");
    cardWidget->setStyleSheet(
        "QWidget#cardWidget {"
        "   background: rgba(255, 255, 255, 0.5);"
        "   border-radius: 25px;"
        "   border: none;"
        "}"
    );

    QVBoxLayout* cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(20, 20, 20, 20);

    QLabel* titleLabel = new QLabel("NCM Converter", cardWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #2b2f4c; background: transparent;");
    cardLayout->addWidget(titleLabel, 1);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addWidget(cardWidget);
    setLayout(mainLayout);
}

void SplashScreen::setStatus(const QString& status)
{
    Q_UNUSED(status);
}

void SplashScreen::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    painter.setClipPath(path);

    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0.0, "#c0d9ff");
    gradient.setColorAt(0.35, "#e0c8ff");
    gradient.setColorAt(0.7, "#ffe6c7");
    gradient.setColorAt(1.0, "#b8e1fc");
    painter.fillRect(rect(), gradient);
}
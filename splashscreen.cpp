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
#include "splashscreen.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QPainter>
#include <QPainterPath>

SplashScreen::SplashScreen(QWidget* parent)
    : QWidget(parent), step(0)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(450, 280);

    QWidget* cardWidget = new QWidget(this);
    cardWidget->setObjectName("cardWidget");

    QVBoxLayout* cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel* titleLabel = new QLabel("NCM Converter", cardWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: white; background: transparent;");
    cardLayout->addWidget(titleLabel, 1);

    // 左下角状态标签
    statusLabel = new QLabel("加载中...", cardWidget);
    statusLabel->setStyleSheet(
        "color: rgba(255,255,255,0.7); "
        "font-size: 12px; "
        "background: transparent;"
    );
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(statusLabel);
    bottomLayout->addStretch();
    cardLayout->addLayout(bottomLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addWidget(cardWidget);
    setLayout(mainLayout);

    // ---- 计时器控制文字切换 ----
    timer = new QTimer(this);
    timer->setSingleShot(false);
    connect(timer, &QTimer::timeout, this, &SplashScreen::updateStatus);
    timer->start(1000);   // 每秒触发一次
}

void SplashScreen::updateStatus()
{
    switch (step) {
    case 0: // 第1秒：显示“加载中...”
        statusLabel->setText("加载中...");
        break;
    case 1: // 第2秒：显示“加载完成”
        statusLabel->setText("加载完成");
        break;
    case 2: // 第3秒：准备隐藏
        statusLabel->setText("加载完成"); // 继续显示
        break;
    case 3: // 第4秒：清空文字
        statusLabel->setText("");
        break;
    case 4: // 第5秒：停止计时器
        timer->stop();
        break;
    }
    step++;
}

void SplashScreen::setStatus(const QString& status)
{
    // 此函数被外部调用，但内部计时器会覆盖，故注释掉
    // statusLabel->setText(status);
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
    gradient.setColorAt(0.0, "#0a0a2a");
    gradient.setColorAt(0.5, "#0d0d3a");
    gradient.setColorAt(1.0, "#050515");
    painter.fillRect(rect(), gradient);
}
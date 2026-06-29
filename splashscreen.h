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
#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class SplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SplashScreen(QWidget* parent = nullptr);

    void setStatus(const QString& status);   // 保留但已禁用

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateStatus();   // 计时器槽

private:
    QLabel* statusLabel;
    QTimer* timer;
    int step;              // 当前步骤
};

#endif
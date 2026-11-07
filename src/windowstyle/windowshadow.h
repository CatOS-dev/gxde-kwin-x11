/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QColor>
#include <QImage>
#include <QMarginsF>
#include <QObject>
#include <QPointF>

#include <array>

namespace KWin
{

class Window;

class WindowShadow : public QObject
{
    Q_OBJECT

public:
    explicit WindowShadow(Window *window);

    bool updateWindowShadow();
    bool isValid() const;
    const QImage &element(int index) const;
    QMarginsF padding() const;

private:
    struct ShadowConfig
    {
        QPointF windowRadius;
        QPointF shadowOffset;
        QColor shadowColor;
        qreal shadowRadius = 0;
        qreal borderWidth = 0;
        QColor borderColor;
    };

    ShadowConfig config() const;
    QString cacheKey(const ShadowConfig &config) const;
    bool isSpecialWindow() const;
    void clear();
    void generate(const ShadowConfig &config);

    Window *m_window;
    std::array<QImage, 8> m_elements;
    QMarginsF m_padding;
    QString m_key;
};

} // namespace KWin

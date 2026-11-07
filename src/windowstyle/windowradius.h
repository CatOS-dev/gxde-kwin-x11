/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QPointF>

namespace KWin
{

class Window;

class WindowRadius : public QObject
{
    Q_OBJECT

public:
    explicit WindowRadius(Window *window);

    bool updateWindowRadius();
    QPointF windowRadius() const;

private:
    QPointF effectiveWindowRadius() const;

    Window *m_window;
    QPointF m_radius;
};

} // namespace KWin

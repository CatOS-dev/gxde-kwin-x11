/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "splitbar.h"

#include "core/output.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace KWin
{

SplitBar::SplitBar(Output *output)
    : m_output(output)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::SizeHorCursor);
    setWindowOpacity(0);
    setWindowTitle(QStringLiteral("gxde-kwin_x11-splitbar-%1").arg(output->name()));
    setProperty("__gxde_kwin_splitbar", true);
}

Output *SplitBar::output() const
{
    return m_output;
}

void SplitBar::enterEvent(QEnterEvent *event)
{
    setWindowOpacity(1);
    QWidget::enterEvent(event);
}

void SplitBar::leaveEvent(QEvent *event)
{
    setWindowOpacity(0);
    QWidget::leaveEvent(event);
}

void SplitBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        Q_EMIT positionChanged(m_output, event->globalPosition(), false);
    }
    QWidget::mouseMoveEvent(event);
}

void SplitBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Q_EMIT positionChanged(m_output, event->globalPosition(), true);
    }
    QWidget::mouseReleaseEvent(event);
}

void SplitBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF handleRect(6, height() / 2.0 - 50, 8, 100);
    QPainterPath path;
    path.addRoundedRect(handleRect, 4, 4);
    painter.fillPath(path, QColor(128, 128, 128));

    QWidget::paintEvent(event);
}

} // namespace KWin

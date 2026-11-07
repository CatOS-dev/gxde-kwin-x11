/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QWidget>

namespace KWin
{

class Output;

class SplitBar : public QWidget
{
    Q_OBJECT

public:
    explicit SplitBar(Output *output);

    Output *output() const;

Q_SIGNALS:
    void positionChanged(KWin::Output *output, const QPointF &globalPosition, bool finished);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    Output *m_output;
};

} // namespace KWin

/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>

namespace KWin
{

class Output;
class SplitBar;
class Tile;
class Window;

class SplitManage : public QObject
{
    Q_OBJECT

public:
    explicit SplitManage(QObject *parent = nullptr);
    ~SplitManage() override;

    int splitWindowCount(const Window *window) const;

private Q_SLOTS:
    void addWindow(KWin::Window *window);
    void removeWindow(KWin::Window *window);
    void addOutput(KWin::Output *output);
    void removeOutput(KWin::Output *output);
    void updateOutput(KWin::Output *output);
    void resizeSplit(KWin::Output *output, const QPointF &globalPosition, bool finished);

private:
    QList<Window *> splitWindows(Output *output) const;
    void scheduleOutputUpdate(Output *output);

    QHash<Output *, QPointer<SplitBar>> m_bars;
    QSet<Output *> m_pendingOutputs;
};

} // namespace KWin

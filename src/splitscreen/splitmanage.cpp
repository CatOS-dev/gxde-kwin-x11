/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "splitmanage.h"

#include "core/output.h"
#include "splitbar.h"
#include "tiles/tile.h"
#include "tiles/tilemanager.h"
#include "virtualdesktops.h"
#include "window.h"
#include "workspace.h"

#include <QTimer>

namespace KWin
{

SplitManage::SplitManage(QObject *parent)
    : QObject(parent)
{
    connect(workspace(), &Workspace::windowAdded, this, &SplitManage::addWindow);
    connect(workspace(), &Workspace::windowRemoved, this, &SplitManage::removeWindow);
    connect(workspace(), &Workspace::outputAdded, this, &SplitManage::addOutput);
    connect(workspace(), &Workspace::outputRemoved, this, &SplitManage::removeOutput);
    connect(VirtualDesktopManager::self(), &VirtualDesktopManager::currentChanged, this, [this]() {
        for (Output *output : m_bars.keys()) {
            scheduleOutputUpdate(output);
        }
    });

    for (Window *window : workspace()->windows()) {
        addWindow(window);
    }
    for (Output *output : workspace()->outputOrder()) {
        if (workspace()->tileManager(output)) {
            addOutput(output);
        }
    }
}

SplitManage::~SplitManage()
{
    qDeleteAll(m_bars);
}

void SplitManage::addWindow(Window *window)
{
    if (!window || !window->isClient()) {
        return;
    }

    const auto update = [this, window]() {
        scheduleOutputUpdate(window->output());
    };
    connect(window, &Window::quickTileModeChanged, this, update);
    connect(window, &Window::outputChanged, this, update);
    connect(window, &Window::desktopsChanged, this, update);
    connect(window, &Window::minimizedChanged, this, update);
    connect(window, &Window::fullScreenChanged, this, update);
    connect(window, &Window::activitiesChanged, this, update);
    scheduleOutputUpdate(window->output());
}

void SplitManage::removeWindow(Window *window)
{
    if (!window) {
        return;
    }
    scheduleOutputUpdate(window->output());
}

void SplitManage::addOutput(Output *output)
{
    if (!output || m_bars.contains(output)) {
        return;
    }

    auto *bar = new SplitBar(output);
    m_bars.insert(output, bar);
    connect(bar, &SplitBar::positionChanged, this, &SplitManage::resizeSplit);

    if (TileManager *manager = workspace()->tileManager(output)) {
        if (Tile *left = manager->quickTile(QuickTileFlag::Left)) {
            connect(left, &Tile::windowGeometryChanged, this, [this, output]() {
                scheduleOutputUpdate(output);
            });
            connect(left, &Tile::windowsChanged, this, [this, output]() {
                scheduleOutputUpdate(output);
            });
        }
        if (Tile *right = manager->quickTile(QuickTileFlag::Right)) {
            connect(right, &Tile::windowsChanged, this, [this, output]() {
                scheduleOutputUpdate(output);
            });
        }
    }
    scheduleOutputUpdate(output);
}

void SplitManage::removeOutput(Output *output)
{
    m_pendingOutputs.remove(output);
    delete m_bars.take(output);
}

QList<Window *> SplitManage::splitWindows(Output *output) const
{
    QList<Window *> windows;
    for (Window *window : workspace()->windows()) {
        if (window->output() != output || !window->isClient() || !window->isSplitWindow()) {
            continue;
        }
        if (!window->isOnCurrentDesktop() || !window->isOnCurrentActivity() || window->isMinimized() || window->isFullScreen()) {
            continue;
        }
        windows.append(window);
    }
    return windows;
}

int SplitManage::splitWindowCount(const Window *window) const
{
    return window ? splitWindows(window->output()).size() : 0;
}

void SplitManage::scheduleOutputUpdate(Output *output)
{
    if (!output || m_pendingOutputs.contains(output)) {
        return;
    }
    m_pendingOutputs.insert(output);
    QTimer::singleShot(0, this, [this, output]() {
        m_pendingOutputs.remove(output);
        if (!m_bars.contains(output)) {
            return;
        }
        updateOutput(output);
    });
}

void SplitManage::updateOutput(Output *output)
{
    SplitBar *bar = m_bars.value(output);
    TileManager *manager = workspace()->tileManager(output);
    if (!bar || !manager) {
        return;
    }

    bool hasLeft = false;
    bool hasRight = false;
    const QList<Window *> windows = splitWindows(output);
    for (Window *window : windows) {
        hasLeft |= window->quickTileMode() == QuickTileMode(QuickTileFlag::Left);
        hasRight |= window->quickTileMode() == QuickTileMode(QuickTileFlag::Right);
    }
    if (!hasLeft || !hasRight) {
        bar->hide();
        return;
    }

    Tile *left = manager->quickTile(QuickTileFlag::Left);
    if (!left) {
        bar->hide();
        return;
    }

    const QRectF area = workspace()->clientArea(MaximizeArea, output, VirtualDesktopManager::self()->currentDesktop());
    constexpr int barWidth = 20;
    const int x = qRound(left->absoluteGeometry().right()) - barWidth / 2;
    bar->setGeometry(x, qRound(area.y()), barWidth, qRound(area.height()));
    bar->show();
    bar->raise();
}

void SplitManage::resizeSplit(Output *output, const QPointF &globalPosition, bool finished)
{
    TileManager *manager = workspace()->tileManager(output);
    if (!manager) {
        return;
    }
    Tile *left = manager->quickTile(QuickTileFlag::Left);
    if (!left) {
        return;
    }

    const QRectF outputGeometry = output->geometryF();
    QRectF relativeGeometry = left->relativeGeometry();
    relativeGeometry.setRight((globalPosition.x() - outputGeometry.x()) / outputGeometry.width());
    left->setRelativeGeometry(relativeGeometry);

    if (finished) {
        updateOutput(output);
    }
}

} // namespace KWin

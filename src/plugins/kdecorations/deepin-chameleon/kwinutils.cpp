/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "kwinutils.h"

#include "compositor.h"
#include "workspace.h"

KWinUtils::KWinUtils(QObject *parent)
    : QObject(parent)
{
}

KWinUtils *KWinUtils::instance()
{
    static KWinUtils utils;
    return &utils;
}

QObject *KWinUtils::compositor()
{
    return KWin::Compositor::self();
}

bool KWinUtils::isCompositing()
{
    return KWin::Compositor::compositing();
}

void KWinUtils::showSplitMenu(const QRect &buttonRect, uint32_t windowId)
{
    if (KWin::Workspace::self()) {
        KWin::Workspace::self()->showSplitMenu(buttonRect, windowId);
    }
}

void KWinUtils::hideSplitMenu(bool delayed)
{
    if (KWin::Workspace::self()) {
        KWin::Workspace::self()->hideSplitMenu(delayed);
    }
}

void KWinUtils::setSplitMenuKeepShowing(bool keep)
{
    if (KWin::Workspace::self()) {
        KWin::Workspace::self()->setSplitMenuKeepShowing(keep);
    }
}

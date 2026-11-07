/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QObject>
#include <QRect>

class KWinUtils : public QObject
{
    Q_OBJECT

public:
    static KWinUtils *instance();
    static QObject *compositor();
    static bool isCompositing();
    static void showSplitMenu(const QRect &buttonRect, uint32_t windowId);
    static void hideSplitMenu(bool delayed);
    static void setSplitMenuKeepShowing(bool keep);

private:
    explicit KWinUtils(QObject *parent = nullptr);
};

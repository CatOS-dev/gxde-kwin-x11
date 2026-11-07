/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "chameleonconfig.h"

#include "chameleontheme.h"
#include "workspace.h"

#include <QApplication>
#include <QPalette>

qreal ChameleonConfig::m_titlebarHeight = 50;

ChameleonConfig *ChameleonConfig::instance()
{
    static ChameleonConfig config;
    return &config;
}

ChameleonConfig::ChameleonConfig(QObject *parent)
    : QObject(parent)
{
    updateTheme();
    if (KWin::Workspace::self()) {
        connect(KWin::Workspace::self(), &KWin::Workspace::osThemeChanged, this, &ChameleonConfig::updateTheme);
    }
}

QString ChameleonConfig::theme() const
{
    return m_theme;
}

bool ChameleonConfig::setTheme(const QString &theme)
{
    if (theme == m_theme) {
        return true;
    }
    if (!ChameleonTheme::instance()->setTheme(theme)) {
        return false;
    }
    m_theme = theme;
    Q_EMIT themeChanged(m_theme);
    return true;
}

void ChameleonConfig::updateTheme()
{
    const bool dark = KWin::Workspace::self()
        ? KWin::Workspace::self()->isDarkTheme()
        : qApp->palette().color(QPalette::Window).lightness() < 128;
    setTheme(dark ? QStringLiteral("dark/deepin") : QStringLiteral("light/deepin"));
}

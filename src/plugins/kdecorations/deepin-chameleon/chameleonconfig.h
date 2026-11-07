/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "effect/effecthandler.h"

#include <QObject>
#include <QString>

class ChameleonConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)

public:
    enum EffectDataRole {
        WindowRadiusRole = KWin::DataRole::WindowRadiusRole,
        WindowClipPathRole = KWin::DataRole::WindowClipPathRole,
        WindowMaskTextureRole = KWin::DataRole::WindowMaskTextureRole,
        ShadowMaskRole = KWin::DataRole::ShadowMaskRole,
        ShadowOffsetRole = KWin::DataRole::ShadowOffsetRole,
    };

    static ChameleonConfig *instance();

    QString theme() const;
    bool setTheme(const QString &theme);

    static qreal m_titlebarHeight;

Q_SIGNALS:
    void themeChanged(const QString &theme);
    void titlebarHeightChanged();

private:
    explicit ChameleonConfig(QObject *parent = nullptr);
    void updateTheme();

    QString m_theme;
};

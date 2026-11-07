/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QColor>
#include <QMarginsF>
#include <QObject>
#include <QPointF>

namespace KWin
{
class DecorationStyle;
class Window;
}

class ChameleonWindowTheme : public QObject
{
    Q_OBJECT

public:
    enum PropertyFlag : int {
        ThemeProperty = 0x02,
        WindowRadiusProperty = 0x04,
        BorderWidthProperty = 0x08,
        BorderColorProperty = 0x10,
        ShadowRadiusProperty = 0x20,
        ShadowOffsetProperty = 0x40,
        ShadowColorProperty = 0x80,
        MouseInputAreaMargins = 0x100,
        WindowPixelRatioProperty = 0x200,
    };
    Q_DECLARE_FLAGS(PropertyFlags, PropertyFlag)
    Q_FLAG(PropertyFlags)

    explicit ChameleonWindowTheme(KWin::Window *window, QObject *parent = nullptr);

    PropertyFlags validProperties() const;
    bool propertyIsValid(PropertyFlag property) const;
    QString theme() const;
    QPointF windowRadius() const;
    qreal borderWidth() const;
    QColor borderColor() const;
    qreal shadowRadius() const;
    QPointF shadowOffset() const;
    QColor shadowColor() const;
    QMarginsF mouseInputAreaMargins() const;
    qreal windowPixelRatio() const;

Q_SIGNALS:
    void validPropertiesChanged(qint64 properties);
    void themeChanged();
    void windowRadiusChanged();
    void borderWidthChanged();
    void borderColorChanged();
    void shadowRadiusChanged();
    void shadowOffectChanged();
    void shadowColorChanged();
    void mouseInputAreaMarginsChanged();
    void windowPixelRatioChanged();

private:
    KWin::DecorationStyle *m_style = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ChameleonWindowTheme::PropertyFlags)

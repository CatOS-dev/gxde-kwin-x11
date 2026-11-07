/*
 * SPDX-FileCopyrightText: 2017-2024 Deepin Technology Co., Ltd.
 * SPDX-FileCopyrightText: 2026 GXDE contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "chameleonwindowtheme.h"

#include "window.h"
#include "windowstyle/decorationstyle.h"

ChameleonWindowTheme::ChameleonWindowTheme(KWin::Window *window, QObject *parent)
    : QObject(parent)
    , m_style(window ? window->windowStyleObj() : nullptr)
{
    if (!m_style) {
        return;
    }
    connect(m_style, &KWin::DecorationStyle::validPropertiesChanged, this, &ChameleonWindowTheme::validPropertiesChanged);
    connect(m_style, &KWin::DecorationStyle::themeChanged, this, &ChameleonWindowTheme::themeChanged);
    connect(m_style, &KWin::DecorationStyle::windowRadiusChanged, this, &ChameleonWindowTheme::windowRadiusChanged);
    connect(m_style, &KWin::DecorationStyle::borderWidthChanged, this, &ChameleonWindowTheme::borderWidthChanged);
    connect(m_style, &KWin::DecorationStyle::borderColorChanged, this, &ChameleonWindowTheme::borderColorChanged);
    connect(m_style, &KWin::DecorationStyle::shadowRadiusChanged, this, &ChameleonWindowTheme::shadowRadiusChanged);
    connect(m_style, &KWin::DecorationStyle::shadowOffsetChanged, this, &ChameleonWindowTheme::shadowOffectChanged);
    connect(m_style, &KWin::DecorationStyle::shadowColorChanged, this, &ChameleonWindowTheme::shadowColorChanged);
    connect(m_style, &KWin::DecorationStyle::mouseInputAreaMarginsChanged, this, &ChameleonWindowTheme::mouseInputAreaMarginsChanged);
    connect(m_style, &KWin::DecorationStyle::windowPixelRatioChanged, this, &ChameleonWindowTheme::windowPixelRatioChanged);
}

ChameleonWindowTheme::PropertyFlags ChameleonWindowTheme::validProperties() const
{
    return m_style ? PropertyFlags::fromInt(m_style->validProperties().toInt()) : PropertyFlags();
}

bool ChameleonWindowTheme::propertyIsValid(PropertyFlag property) const
{
    return validProperties().testFlag(property);
}

QString ChameleonWindowTheme::theme() const
{
    return m_style ? m_style->theme() : QString();
}

QPointF ChameleonWindowTheme::windowRadius() const
{
    return m_style ? m_style->windowRadius() : QPointF();
}

qreal ChameleonWindowTheme::borderWidth() const
{
    return m_style ? m_style->borderWidth() : 0;
}

QColor ChameleonWindowTheme::borderColor() const
{
    return m_style ? m_style->borderColor() : QColor();
}

qreal ChameleonWindowTheme::shadowRadius() const
{
    return m_style ? m_style->shadowRadius() : 0;
}

QPointF ChameleonWindowTheme::shadowOffset() const
{
    return m_style ? m_style->shadowOffset() : QPointF();
}

QColor ChameleonWindowTheme::shadowColor() const
{
    return m_style ? m_style->shadowColor() : QColor();
}

QMarginsF ChameleonWindowTheme::mouseInputAreaMargins() const
{
    return m_style ? m_style->mouseInputAreaMargins() : QMarginsF();
}

qreal ChameleonWindowTheme::windowPixelRatio() const
{
    return m_style ? m_style->windowPixelRatio() : 1;
}

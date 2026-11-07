/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "decorationstyle.h"

#include "config-kwin.h"
#include "window.h"
#if KWIN_BUILD_X11
#include "atoms.h"
#include "utils/xcbutils.h"
#include "x11window.h"
#endif

#include <QApplication>
#include <QVariant>

namespace KWin
{

#if KWIN_BUILD_X11
static xcb_window_t x11WindowId(Window *window)
{
    if (const auto x11Window = qobject_cast<X11Window *>(window)) {
        return x11Window->window();
    }
    return XCB_WINDOW_NONE;
}
#endif

DecorationStyle::DecorationStyle(Window *window)
    : QObject(window)
    , m_window(window)
{
    static const QFunctionPointer buildNativeSettings = qApp->platformFunction("_d_buildNativeSettings");
#if KWIN_BUILD_X11
    const xcb_window_t windowId = x11WindowId(window);
    if (buildNativeSettings && windowId != XCB_WINDOW_NONE) {
        reinterpret_cast<bool (*)(QObject *, quint32)>(buildNativeSettings)(this, windowId);
    }
#endif
    syncX11Properties();
}

DecorationStyle::PropertyFlags DecorationStyle::validProperties() const
{
    return m_validProperties;
}

bool DecorationStyle::propertyIsValid(PropertyFlag property) const
{
    return m_validProperties.testFlag(property);
}

QString DecorationStyle::theme() const
{
    return m_theme;
}

QPointF DecorationStyle::windowRadius() const
{
    return m_windowRadius;
}

qreal DecorationStyle::borderWidth() const
{
    return m_borderWidth;
}

QColor DecorationStyle::borderColor() const
{
    return m_borderColor;
}

qreal DecorationStyle::shadowRadius() const
{
    return m_shadowRadius;
}

QPointF DecorationStyle::shadowOffset() const
{
    return m_shadowOffset;
}

QColor DecorationStyle::shadowColor() const
{
    return m_shadowColor;
}

QMarginsF DecorationStyle::mouseInputAreaMargins() const
{
    return m_mouseInputAreaMargins;
}

qreal DecorationStyle::windowPixelRatio() const
{
    return propertyIsValid(WindowPixelRatioProperty) ? m_windowPixelRatio : 1;
}

qint64 DecorationStyle::windowEffect() const
{
    return m_windowEffect;
}

qreal DecorationStyle::windowStartUpEffect() const
{
    return m_windowStartUpEffect;
}

bool DecorationStyle::noTitleBar() const
{
    return m_noTitleBar;
}

DecorationStyle::EffectScenes DecorationStyle::effectScenes() const
{
    return EffectScenes::fromInt(static_cast<int>(m_windowEffect));
}

bool DecorationStyle::isCancelShadow() const
{
    return m_cancelShadow;
}

bool DecorationStyle::isCancelRadius() const
{
    return m_cancelRadius;
}

void DecorationStyle::cancelShadowByUser(bool cancel)
{
    m_cancelShadow = cancel;
}

void DecorationStyle::cancelRadiusByUser(bool cancel)
{
    m_cancelRadius = cancel;
}

QPointF DecorationStyle::variantToPoint(const QVariant &value, const QPointF &fallback) const
{
    if (value.canConvert<QPointF>()) {
        return value.toPointF();
    }
    const QStringList parts = value.toString().split(QLatin1Char(','));
    if (parts.size() < 2) {
        return fallback;
    }
    return QPointF(parts.at(0).toDouble(), parts.at(1).toDouble());
}

QMarginsF DecorationStyle::variantToMargins(const QVariant &value, const QMarginsF &fallback) const
{
    if (value.canConvert<QMarginsF>()) {
        return value.value<QMarginsF>();
    }
    QStringList parts = value.toStringList();
    if (parts.isEmpty()) {
        parts = value.toString().split(QLatin1Char(','));
    }
    if (parts.size() < 4) {
        return fallback;
    }
    return QMarginsF(parts.at(0).toDouble(), parts.at(1).toDouble(), parts.at(2).toDouble(), parts.at(3).toDouble());
}

void DecorationStyle::syncX11Properties()
{
    if (property("validProperties").isValid()) {
        setValidProperties(property("validProperties").toLongLong());
    }
    if (propertyIsValid(ThemeProperty)) {
        setTheme(property("theme").toString());
    }
    if (propertyIsValid(WindowRadiusProperty)) {
        setWindowRadius(variantToPoint(property("windowRadius")));
    }
    if (propertyIsValid(BorderWidthProperty)) {
        setBorderWidth(property("borderWidth").toDouble());
    }
    if (propertyIsValid(BorderColorProperty)) {
        setBorderColor(property("borderColor").value<QColor>());
    }
    if (propertyIsValid(ShadowRadiusProperty)) {
        setShadowRadius(property("shadowRadius").toDouble());
    }
    if (propertyIsValid(ShadowOffsetProperty)) {
        setShadowOffset(variantToPoint(property("shadowOffset")));
    }
    if (propertyIsValid(ShadowColorProperty)) {
        setShadowColor(property("shadowColor").value<QColor>());
    }
    if (propertyIsValid(MouseInputAreaMarginsProperty)) {
        setMouseInputAreaMargins(variantToMargins(property("mouseInputAreaMargins")));
    }
    if (propertyIsValid(WindowPixelRatioProperty)) {
        setWindowPixelRatio(property("windowPixelRatio").toDouble());
    }
    if (propertyIsValid(WindowEffectProperty)) {
        setWindowEffect(property("windowEffect").toLongLong());
    }
    if (propertyIsValid(WindowStartUpEffectProperty)) {
        setWindowStartUpEffect(property("windowStartUpEffect").toDouble());
    }
    if (property("noTitleBar").isValid()) {
        setNoTitleBar(property("noTitleBar").toBool());
    }

#if KWIN_BUILD_X11
    const xcb_window_t windowId = x11WindowId(m_window);
    if (!atoms || windowId == XCB_WINDOW_NONE) {
        return;
    }

    Xcb::Property noTitleBar(false, windowId, atoms->deepin_no_titlebar, XCB_ATOM_CARDINAL, 0, 1);
    if (const auto value = noTitleBar.value<const quint32 *>()) {
        setNoTitleBar(*value != 0);
    } else {
        setNoTitleBar(false);
    }

    if (!propertyIsValid(WindowEffectProperty)) {
        Xcb::Property effect(false, windowId, atoms->deepin_net_effect, XCB_ATOM_CARDINAL, 0, 1);
        if (const auto value = effect.value<const quint32 *>()) {
            setWindowEffect(*value);
        } else {
            setWindowEffect(0);
        }
    }
    const EffectScenes scenes = effectScenes();
    m_cancelRadius = scenes.testFlag(EffectNoRadius);
    m_cancelShadow = scenes.testFlag(EffectNoShadow);

    if (!m_cancelRadius && (!propertyIsValid(WindowRadiusProperty) || m_x11RadiusProperty)) {
        Xcb::StringProperty radius(windowId, atoms->deepin_net_radius);
        const QByteArray data = radius.toByteArray();
        if (!data.isEmpty()) {
            setValidProperties(validProperties().toInt() | WindowRadiusProperty);
            setWindowRadius(variantToPoint(QString::fromUtf8(data)));
            m_x11RadiusProperty = true;
        } else if (m_x11RadiusProperty) {
            setValidProperties(validProperties().toInt() & ~WindowRadiusProperty);
            setWindowRadius(QPointF());
            m_x11RadiusProperty = false;
        }
    }
    if (!m_cancelShadow && (!propertyIsValid(ShadowColorProperty) || m_x11ShadowProperty)) {
        Xcb::StringProperty shadow(windowId, atoms->deepin_net_shadow);
        const QByteArray data = shadow.toByteArray();
        if (!data.isEmpty()) {
            setValidProperties(validProperties().toInt() | ShadowColorProperty);
            setShadowColor(QColor(QString::fromUtf8(data)));
            m_x11ShadowProperty = true;
        } else if (m_x11ShadowProperty) {
            setValidProperties(validProperties().toInt() & ~ShadowColorProperty);
            setShadowColor(QColor());
            m_x11ShadowProperty = false;
        }
    }
#endif
}

#define GXDE_STYLE_SETTER(Method, Member, Signal, Type) \
    void DecorationStyle::Method(Type value)            \
    {                                                   \
        if (Member == value) {                          \
            return;                                     \
        }                                               \
        Member = value;                                 \
        Q_EMIT Signal();                                \
    }

void DecorationStyle::setValidProperties(qint64 properties)
{
    const PropertyFlags flags = PropertyFlags::fromInt(static_cast<int>(properties));
    if (m_validProperties == flags) {
        return;
    }
    m_validProperties = flags;
    Q_EMIT validPropertiesChanged(properties);
}

GXDE_STYLE_SETTER(setTheme, m_theme, themeChanged, const QString &)
GXDE_STYLE_SETTER(setWindowRadius, m_windowRadius, windowRadiusChanged, const QPointF &)
GXDE_STYLE_SETTER(setBorderWidth, m_borderWidth, borderWidthChanged, qreal)
GXDE_STYLE_SETTER(setBorderColor, m_borderColor, borderColorChanged, const QColor &)
GXDE_STYLE_SETTER(setShadowRadius, m_shadowRadius, shadowRadiusChanged, qreal)
GXDE_STYLE_SETTER(setShadowOffset, m_shadowOffset, shadowOffsetChanged, const QPointF &)
GXDE_STYLE_SETTER(setShadowColor, m_shadowColor, shadowColorChanged, const QColor &)
GXDE_STYLE_SETTER(setMouseInputAreaMargins, m_mouseInputAreaMargins, mouseInputAreaMarginsChanged, const QMarginsF &)
GXDE_STYLE_SETTER(setWindowPixelRatio, m_windowPixelRatio, windowPixelRatioChanged, qreal)
GXDE_STYLE_SETTER(setWindowStartUpEffect, m_windowStartUpEffect, windowStartUpEffectChanged, qreal)
GXDE_STYLE_SETTER(setNoTitleBar, m_noTitleBar, noTitleBarChanged, bool)

#undef GXDE_STYLE_SETTER

void DecorationStyle::setWindowEffect(qint64 effect)
{
    if (m_windowEffect == effect) {
        return;
    }
    m_windowEffect = effect;
    const EffectScenes scenes = effectScenes();
    m_cancelRadius = scenes.testFlag(EffectNoRadius);
    m_cancelShadow = scenes.testFlag(EffectNoShadow);
    Q_EMIT windowEffectChanged();
    Q_EMIT windowRadiusChanged();
}

} // namespace KWin

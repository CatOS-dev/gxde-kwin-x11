/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "windowshadow.h"

#include "rules.h"
#include "window.h"
#include "windowstyle/decorationstyle.h"
#include "windowstyle/windowradius.h"
#include "windowstyle/windowstylemanager.h"
#include "workspace.h"
#if KWIN_BUILD_X11
#include "x11window.h"
#endif

#include <QPainter>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>

namespace KWin
{

struct CachedShadow
{
    std::array<QImage, 8> elements;
    QMarginsF padding;
};

static QHash<QString, CachedShadow> s_shadowCache;

WindowShadow::WindowShadow(Window *window)
    : QObject(window)
    , m_window(window)
{
    DecorationStyle *style = m_window->windowStyleObj();
    connect(style, &DecorationStyle::borderWidthChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
    connect(style, &DecorationStyle::borderColorChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
    connect(style, &DecorationStyle::shadowRadiusChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
    connect(style, &DecorationStyle::shadowOffsetChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
    connect(style, &DecorationStyle::shadowColorChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
    connect(style, &DecorationStyle::windowEffectChanged, this, [this]() {
        m_window->updateWindowShadow();
    });
}

bool WindowShadow::updateWindowShadow()
{
    if (!m_window || !m_window->windowStyleObj()) {
        return false;
    }

    const DecorationStyle *style = m_window->windowStyleObj();
    bool hasClientAlpha = false;
#if KWIN_BUILD_X11
    if (const auto *x11Window = dynamic_cast<const X11Window *>(m_window)) {
        hasClientAlpha = x11Window->hasAlpha();
    }
#endif
    const bool disableCorner = m_window->rules() && m_window->rules()->checkDisableCorner(false);
    const bool forceShadow = m_window->rules() && m_window->rules()->checkEnableWindowShadow(false);
    if (m_window->maximizeMode() == MaximizeFull
        || m_window->isFullScreen()
        || m_window->isDesktop()
        || disableCorner
        || (!forceShadow && (style->isCancelShadow() || (hasClientAlpha && !m_window->borderRedrawable() && !style->propertyIsValid(DecorationStyle::WindowRadiusProperty))))) {
        const bool changed = isValid();
        clear();
        return changed;
    }

    const ShadowConfig shadowConfig = config();
    const QString key = cacheKey(shadowConfig);
    if (key == m_key) {
        return false;
    }
    m_key = key;

    const auto cached = s_shadowCache.constFind(key);
    if (cached != s_shadowCache.constEnd()) {
        m_elements = cached->elements;
        m_padding = cached->padding;
        return true;
    }

    generate(shadowConfig);
    if (s_shadowCache.size() >= 32) {
        s_shadowCache.clear();
    }
    s_shadowCache.insert(key, CachedShadow{m_elements, m_padding});
    return true;
}

bool WindowShadow::isValid() const
{
    return !m_key.isEmpty();
}

const QImage &WindowShadow::element(int index) const
{
    return m_elements.at(index);
}

QMarginsF WindowShadow::padding() const
{
    return m_padding;
}

WindowShadow::ShadowConfig WindowShadow::config() const
{
    const DecorationStyle *style = m_window->windowStyleObj();
    const qreal scale = workspace()->windowStyleManager()->osScale();
    const bool dark = workspace()->windowStyleManager()->isDarkTheme();
    const bool special = isSpecialWindow();

    ShadowConfig result;
    if (m_window->windowRadiusObj()) {
        result.windowRadius = m_window->windowRadiusObj()->windowRadius();
    }
    result.shadowOffset = style->propertyIsValid(DecorationStyle::ShadowOffsetProperty)
        ? style->shadowOffset()
        : QPointF(0, (special ? 6 : 22) * scale);
    const qreal extentLimit = 256 * scale;
    result.windowRadius.setX(std::clamp(result.windowRadius.x(), qreal(0), extentLimit));
    result.windowRadius.setY(std::clamp(result.windowRadius.y(), qreal(0), extentLimit));
    result.shadowOffset.setX(std::clamp(result.shadowOffset.x(), -extentLimit, extentLimit));
    result.shadowOffset.setY(std::clamp(result.shadowOffset.y(), -extentLimit, extentLimit));
    if (style->propertyIsValid(DecorationStyle::ShadowColorProperty)) {
        result.shadowColor = style->shadowColor();
    } else if (m_window->isActive()) {
        result.shadowColor = QColor(dark ? QStringLiteral("#a9000000") : QStringLiteral("#80000000"));
    } else if (dark) {
        result.shadowColor = QColor(special ? QStringLiteral("#80000000") : QStringLiteral("#69000000"));
    } else {
        result.shadowColor = QColor(special ? QStringLiteral("#33000000") : QStringLiteral("#40000000"));
    }
    result.shadowRadius = style->propertyIsValid(DecorationStyle::ShadowRadiusProperty)
        ? style->shadowRadius()
        : (special ? 12 : 50) * scale;
    result.shadowRadius = std::clamp(result.shadowRadius, qreal(0), extentLimit);
    if (style->effectScenes().testFlag(DecorationStyle::EffectNoBorder)) {
        result.borderWidth = 0;
    } else {
        result.borderWidth = style->propertyIsValid(DecorationStyle::BorderWidthProperty)
            ? style->borderWidth()
            : scale;
    }
    result.borderWidth = std::clamp(result.borderWidth, qreal(0), 32 * scale);
    result.borderColor = style->propertyIsValid(DecorationStyle::BorderColorProperty)
        ? style->borderColor()
        : QColor(dark ? QStringLiteral("#1affffff") : QStringLiteral("#10000000"));
    return result;
}

QString WindowShadow::cacheKey(const ShadowConfig &config) const
{
    return QStringLiteral("%1,%2;%3,%4;%5;%6;%7;%8")
        .arg(config.windowRadius.x())
        .arg(config.windowRadius.y())
        .arg(config.shadowOffset.x())
        .arg(config.shadowOffset.y())
        .arg(config.shadowColor.rgba())
        .arg(config.shadowRadius)
        .arg(config.borderWidth)
        .arg(config.borderColor.rgba());
}

bool WindowShadow::isSpecialWindow() const
{
    return m_window->isPopupMenu() || m_window->isMenu() || m_window->isTooltip() || m_window->isDropdownMenu();
}

void WindowShadow::clear()
{
    m_key.clear();
    m_padding = QMarginsF();
    for (QImage &element : m_elements) {
        element = QImage();
    }
}

void WindowShadow::generate(const ShadowConfig &config)
{
    const int shadowSize = std::max(1, qCeil(config.shadowRadius + config.windowRadius.x() + config.windowRadius.y()));
    const QPointF overlap(std::max<qreal>(config.windowRadius.x(), 3),
                          std::max<qreal>(config.windowRadius.y(), 3));
    m_padding = QMarginsF(shadowSize - config.shadowOffset.x() - overlap.x(),
                          shadowSize - config.shadowOffset.y() - overlap.y(),
                          shadowSize - overlap.x(),
                          shadowSize - overlap.y());

    QImage image(2 * shadowSize, 2 * shadowSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    const bool drawShadow = config.shadowRadius > 0 && config.shadowColor.alpha() > 0;
    if (drawShadow) {
        QRadialGradient gradient(shadowSize, shadowSize, shadowSize);
        for (int i = 0; i < 10; ++i) {
            const qreal x = qreal(i) / 9;
            QColor color = config.shadowColor;
            color.setAlphaF(std::clamp(std::exp(-x * x / 0.15) * config.shadowColor.alphaF() * 0.6, 0.0, 1.0));
            gradient.setColorAt(x, color);
        }
        QColor transparent = config.shadowColor;
        transparent.setAlpha(0);
        gradient.setColorAt(1, transparent);
        QPainter painter(&image);
        painter.fillRect(image.rect(), gradient);
    }

    const QRectF innerRect(shadowSize - config.shadowOffset.x() - overlap.x(),
                           shadowSize - config.shadowOffset.y() - overlap.y(),
                           config.shadowOffset.x() + 2 * overlap.x(),
                           config.shadowOffset.y() + 2 * overlap.y());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (config.borderWidth > 0 && config.borderColor.alpha() > 0) {
        painter.setPen(QPen(config.borderColor, config.borderWidth + 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(innerRect,
                                std::max<qreal>(0, config.windowRadius.x() - 0.5),
                                std::max<qreal>(0, config.windowRadius.y() - 0.5));
    }
    if (drawShadow) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        painter.drawRoundedRect(innerRect, config.windowRadius.x() + 0.5, config.windowRadius.y() + 0.5);
    }
    painter.end();

    const QRect center(shadowSize, shadowSize, 1, 1);
    m_elements[0] = image.copy(QRect(center.left(), 0, 1, center.top()));
    m_elements[1] = image.copy(QRect(center.right() + 1, 0, image.width() - center.right() - 1, center.top()));
    m_elements[2] = image.copy(QRect(center.right() + 1, center.top(), image.width() - center.right() - 1, 1));
    m_elements[3] = image.copy(QRect(center.right() + 1, center.bottom() + 1, image.width() - center.right() - 1, image.height() - center.bottom() - 1));
    m_elements[4] = image.copy(QRect(center.left(), center.bottom() + 1, 1, image.height() - center.bottom() - 1));
    m_elements[5] = image.copy(QRect(0, center.bottom() + 1, center.left(), image.height() - center.bottom() - 1));
    m_elements[6] = image.copy(QRect(0, center.top(), center.left(), 1));
    m_elements[7] = image.copy(QRect(0, 0, center.left(), center.top()));
}

} // namespace KWin

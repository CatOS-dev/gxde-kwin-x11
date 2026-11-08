/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "windowradius.h"

#include "config-kwin.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"
#include "rules.h"
#include "scene/borderradius.h"
#include "window.h"
#include "windowstyle/decorationstyle.h"
#include "windowstyle/windowstylemanager.h"
#include "workspace.h"
#if KWIN_BUILD_X11
#include "atoms.h"
#endif

#include <QDataStream>
#include <QPainterPath>

#include <algorithm>

Q_DECLARE_METATYPE(QPainterPath)

namespace KWin
{

WindowRadius::WindowRadius(Window *window)
    : QObject(window)
    , m_window(window)
{
    connect(m_window->windowStyleObj(), &DecorationStyle::windowRadiusChanged, this, [this]() {
        updateWindowRadius();
    });
}

bool WindowRadius::updateWindowRadius()
{
    if (!m_window || !m_window->windowStyleObj()) {
        return false;
    }

    QPointF radius = effectiveWindowRadius();
    QPainterPath clipPath;

#if KWIN_BUILD_X11
    if (atoms && m_window->effectWindow()) {
        const QByteArray clipData = m_window->effectWindow()->readProperty(atoms->deepin_scissor_window,
                                                                           atoms->deepin_scissor_window,
                                                                           8);
        if (!clipData.isEmpty() && clipData.size() <= 1024 * 1024) {
            QDataStream stream(clipData);
            stream >> clipPath;
        }
    }
#endif

    EffectWindow *effect = m_window->effectWindow();
    if (effect) {
        if (clipPath.isEmpty()) {
            effect->setData(WindowClipPathRole, QVariant());
            effect->setData(WindowRadiusRole, radius.isNull() ? QVariant() : QVariant::fromValue(radius));
        } else {
            effect->setData(WindowRadiusRole, QVariant());
            effect->setData(WindowClipPathRole, QVariant::fromValue(clipPath));
            radius = QPointF();
        }
    }

    const qreal nativeRadius = std::max<qreal>(0, std::min(radius.x(), radius.y()));
    m_window->setBorderRadius(BorderRadius(nativeRadius));

    if (m_radius == radius) {
        return false;
    }
    m_radius = radius;
    return true;
}

QPointF WindowRadius::windowRadius() const
{
    return m_radius;
}

QPointF WindowRadius::effectiveWindowRadius() const
{
    if (m_window->rules() && m_window->rules()->checkDisableCorner(false)) {
        return QPointF();
    }
    if (m_window->maximizeMode() == MaximizeFull || m_window->isFullScreen() || m_window->isDesktop()) {
        return QPointF();
    }

    const DecorationStyle *style = m_window->windowStyleObj();
    if (style->isCancelRadius()) {
        return QPointF();
    }
    if (style->propertyIsValid(DecorationStyle::WindowRadiusProperty)) {
        const qreal limit = 256 * workspace()->windowStyleManager()->osScale();
        const QPointF radius = style->windowRadius();
        return QPointF(std::clamp(radius.x(), qreal(0), limit),
                       std::clamp(radius.y(), qreal(0), limit));
    }
    if (!m_window->borderRedrawable()) {
        return QPointF();
    }

    const qreal radius = std::clamp(workspace()->windowStyleManager()->osRadius(), qreal(0), qreal(100))
        * workspace()->windowStyleManager()->osScale();
    return QPointF(radius, radius);
}

} // namespace KWin

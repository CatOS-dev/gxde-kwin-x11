/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "splitswap.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"

namespace KWin
{

namespace SplitConsts
{
const QEasingCurve ToggleMode = QEasingCurve::OutExpo;
constexpr std::chrono::milliseconds FadeDuration{600};
}

SplitSwapEffect::SplitSwapEffect()
{
    reconfigure(ReconfigureAll);
    connect(effectsEx, &EffectsHandlerEx::swapSplitWin, this, &SplitSwapEffect::onSwapWindow);
}

void SplitSwapEffect::reconfigure(ReconfigureFlags flags)
{
    m_duration = std::chrono::milliseconds(static_cast<int>(animationTime(SplitConsts::FadeDuration)));
    m_animationTime.setDuration(m_duration);
    m_animationTime.setDirection(TimeLine::Forward);
    m_animationTime.setEasingCurve(SplitConsts::ToggleMode);
}

void SplitSwapEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    if (isActive()) {
        m_animationTime.advance(presentTime);
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }
    for (EffectWindow *window : effects->stackingOrder()) {
        window->setData(WindowForceBlurRole, true);
    }
    effects->prePaintScreen(data, presentTime);
}

void SplitSwapEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, region, screen);
}

void SplitSwapEffect::postPaintScreen()
{
    effects->postPaintScreen();

    if (m_isFinishing && m_activated && m_animationTime.done()) {
        for (EffectWindow *window : effects->stackingOrder()) {
            if (window->screen() != m_dragScreen) {
                continue;
            }
            const int mode = effectsEx->getQuickTileMode(window);
            if (mode == int(QuickTileFlag::Left)) {
                resetWindowPosition(window, QuickTileMode(m_leftMode));
                effectsEx->updateQuickTileMode(window, m_leftMode);
            } else if (mode == int(QuickTileFlag::Right)) {
                resetWindowPosition(window, QuickTileMode(m_rightMode));
                effectsEx->updateQuickTileMode(window, m_rightMode);
            }
        }
        effectsEx->updateWindowTile(m_dragScreen);
        setActive(false);
    }

    for (EffectWindow *window : effects->stackingOrder()) {
        window->setData(WindowForceBlurRole, QVariant());
    }
    if (m_activated && m_animationTime.running()) {
        effects->addRepaintFull();
    }
}

void SplitSwapEffect::prePaintWindow(EffectWindow *window, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
{
    data.setTransformed();
    effects->prePaintWindow(window, data, presentTime);
}

void SplitSwapEffect::paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask, QRegion region, WindowPaintData &data)
{
    if (!isActive() || window->screen() != m_dragScreen || window->isDesktop()) {
        effects->paintWindow(renderTarget, viewport, window, mask, region, data);
        return;
    }

    if (m_isSwapping && window != m_dragWindow) {
        const int mode = effectsEx->getQuickTileMode(window);
        if (mode == int(QuickTileFlag::Left)) {
            const int x = paintedWindowX(window, QuickTileMode(m_leftMode), 2);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        } else if (mode == int(QuickTileFlag::Right)) {
            const int x = paintedWindowX(window, QuickTileMode(m_rightMode), 2);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        }
    }

    if (m_isFinishing) {
        const int mode = effectsEx->getQuickTileMode(window);
        if (window == m_dragWindow && mode == int(QuickTileFlag::Left)) {
            const int x = paintedWindowX(window, QuickTileMode(m_leftMode), 0);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        } else if (window == m_dragWindow && mode == int(QuickTileFlag::Right)) {
            const int x = paintedWindowX(window, QuickTileMode(m_rightMode), 0);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        } else if (mode == int(QuickTileFlag::Left)) {
            const int x = paintedWindowX(window, QuickTileMode(m_leftMode), 1);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        } else if (mode == int(QuickTileFlag::Right)) {
            const int x = paintedWindowX(window, QuickTileMode(m_rightMode), 1);
            data += QPoint(x - window->x(), m_workArea.y() - window->y());
        }
    }

    effects->paintWindow(renderTarget, viewport, window, mask, region, data);
}

bool SplitSwapEffect::isActive() const
{
    return m_activated && !effects->isScreenLocked();
}

void SplitSwapEffect::setActive(bool active)
{
    if (m_activated == active) {
        return;
    }
    if (active && effects->activeFullScreenEffect() && effects->activeFullScreenEffect() != this) {
        return;
    }

    m_activated = active;
    if (active) {
        effects->setActiveFullScreenEffect(this);
    } else {
        m_isSwapping = false;
        m_isFinishing = false;
        m_leftMode = int(QuickTileFlag::Left);
        m_rightMode = int(QuickTileFlag::Right);
        m_dragWindow = nullptr;
        m_dragScreen = nullptr;
        effects->setActiveFullScreenEffect(nullptr);
    }
    effects->addRepaintFull();
}

void SplitSwapEffect::onSwapWindow(EffectWindow *window, int index)
{
    if (!window) {
        return;
    }

    m_animationTime.reset();
    if (index == 0) {
        if (!m_activated) {
            return;
        }
        m_animationTime.setElapsed(m_duration);
        m_isSwapping = false;
        m_isFinishing = true;
        effects->addRepaintFull();
        return;
    }

    if (index == 2) {
        m_isSwapping = false;
        m_isFinishing = true;
        m_animationTime.setElapsed(std::chrono::milliseconds(1));
    } else if (index == 1) {
        m_isSwapping = true;
        m_isFinishing = false;
        m_leftMode ^= 0b11;
        m_rightMode ^= 0b11;
    } else {
        return;
    }

    m_dragWindow = window;
    if (!m_dragScreen) {
        m_dragScreen = window->screen();
    }
    m_workArea = effects->clientArea(MaximizeArea, window).toRect();
    setActive(true);
    effects->addRepaintFull();
}

void SplitSwapEffect::resetWindowPosition(EffectWindow *window, QuickTileMode mode)
{
    QPoint target = window->frameGeometry().topLeft().toPoint();
    if (mode & QuickTileFlag::Left) {
        target = m_workArea.topLeft();
    } else if (mode & QuickTileFlag::Right) {
        target = QPoint(m_workArea.right() - window->width() + 1, m_workArea.y());
    }
    if (window->frameGeometry().topLeft().toPoint() != target) {
        effects->moveWindow(window, target);
    }
}

int SplitSwapEffect::paintedWindowX(EffectWindow *window, QuickTileMode mode, int calculationMethod) const
{
    qreal coefficient = m_animationTime.value();
    int start = 0;
    int distance = 0;

    switch (calculationMethod) {
    case 0:
        start = qRound(window->x());
        distance = m_workArea.x() + m_workArea.width() - window->width() - window->x();
        break;
    case 1:
        coefficient = 1.0;
        Q_FALLTHROUGH();
    case 2:
        start = (mode & QuickTileFlag::Left) ? m_workArea.x() + m_workArea.width() - window->width() : m_workArea.x();
        distance = m_workArea.width() - window->width();
        break;
    default:
        return qRound(window->x());
    }

    if (mode & QuickTileFlag::Left) {
        return start - (start - m_workArea.x()) * coefficient;
    }
    return start + distance * coefficient;
}

} // namespace KWin

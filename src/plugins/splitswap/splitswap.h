/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 zhang yu <zhangyud@uniontech.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "effect/timeline.h"
#include "kwineffectsex.h"

namespace KWin
{

class SplitSwapEffect : public Effect
{
    Q_OBJECT

public:
    SplitSwapEffect();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen) override;
    void postPaintScreen() override;
    void prePaintWindow(EffectWindow *window, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask, QRegion region, WindowPaintData &data) override;

    bool isActive() const override;

private:
    void setActive(bool active);
    void resetWindowPosition(EffectWindow *window, QuickTileMode mode);
    int paintedWindowX(EffectWindow *window, QuickTileMode mode, int calculationMethod) const;

private Q_SLOTS:
    void onSwapWindow(KWin::EffectWindow *window, int index);

private:
    QRect m_workArea;
    std::chrono::milliseconds m_duration;
    TimeLine m_animationTime;
    bool m_activated = false;
    bool m_isSwapping = true;
    bool m_isFinishing = false;
    int m_leftMode = int(QuickTileFlag::Left);
    int m_rightMode = int(QuickTileFlag::Right);
    EffectWindow *m_dragWindow = nullptr;
    Output *m_dragScreen = nullptr;
};

} // namespace KWin

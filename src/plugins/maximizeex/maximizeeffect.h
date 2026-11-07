/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "effect/effect.h"
#include "effect/effectwindow.h"
#include "effect/timeline.h"

#include <memory>

namespace KWin
{

class GLFramebuffer;
class GLTexture;

class MaximizeEffect : public Effect
{
    Q_OBJECT
public:
    MaximizeEffect();
    ~MaximizeEffect() override;

    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen) override;
    void postPaintScreen() override;
    void prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, QRegion region, WindowPaintData &data) override;
    void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const QRegion &region, WindowPaintData &data) override;

    bool isActive() const override;
    static bool supported();

private:
    void trackWindow(EffectWindow *window);
    void setActive(bool);
    void cleanup();

public Q_SLOTS:
    void slotWindowMaximizedStateChanged(EffectWindow *window, bool horizontal, bool vertical);
    void slotWindowMaximizedStateAboutToChange(EffectWindow *window, bool horizontal, bool vertical);

private:
    bool m_activated = false;
    EffectWindow *m_maxiWin = nullptr;
    QRectF m_oldGeo;
    QRectF m_newGeo;
    TimeLine m_animationTime;
    bool m_isMaximized = false;

    std::unique_ptr<GLFramebuffer> m_fbo;
    std::unique_ptr<GLTexture> m_texture;
};
}

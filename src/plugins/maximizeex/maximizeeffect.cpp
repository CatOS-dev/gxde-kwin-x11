/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "maximizeeffect.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glframebuffer.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "opengl/gltexture.h"
#include "window.h"

namespace MaxiConsts
{
const QEasingCurve TOGGLE_MODE = QEasingCurve::OutQuart;
static constexpr std::chrono::milliseconds FADE_DURATION{350};
}

namespace KWin
{
MaximizeEffect::MaximizeEffect()
{
    connect(effects, &EffectsHandler::windowAdded, this, &MaximizeEffect::trackWindow);
    for (EffectWindow *window : effects->stackingOrder()) {
        trackWindow(window);
    }

    m_animationTime.setDuration(std::chrono::milliseconds(static_cast<int>(animationTime(MaxiConsts::FADE_DURATION))));
    m_animationTime.setDirection(TimeLine::Forward);
    m_animationTime.setEasingCurve(MaxiConsts::TOGGLE_MODE);
}

MaximizeEffect::~MaximizeEffect() = default;

void MaximizeEffect::trackWindow(EffectWindow *window)
{
    connect(window, &EffectWindow::windowMaximizedStateChanged,
            this, &MaximizeEffect::slotWindowMaximizedStateChanged, Qt::UniqueConnection);
    connect(window, &EffectWindow::windowMaximizedStateAboutToChange,
            this, &MaximizeEffect::slotWindowMaximizedStateAboutToChange, Qt::UniqueConnection);
}

void MaximizeEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    if (isActive()) {
        m_animationTime.advance(presentTime);
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    }

    for (auto const &w : effects->stackingOrder()) {
        w->setData(WindowForceBlurRole, QVariant(true));
    }

    effects->prePaintScreen(data, presentTime);
}

void MaximizeEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, region, screen);
}

void MaximizeEffect::postPaintScreen()
{
    if (m_activated && m_animationTime.done()) {
        setActive(false);
    }
    if (m_activated) {
        effects->addRepaintFull();
    }

    for (auto const &w : effects->stackingOrder()) {
        w->setData(WindowForceBlurRole, QVariant());
    }
    effects->postPaintScreen();
}

void MaximizeEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
{
    data.mask |= PAINT_WINDOW_TRANSFORMED;

    effects->prePaintWindow(w, data, presentTime);
}

void MaximizeEffect::paintWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, QRegion region, WindowPaintData &data)
{
    if (!isActive()) {
        effects->paintWindow(renderTarget, viewport, w, mask, region, data);
        return;
    }

    // Deepin watermark windows must not be transformed with their owner.
    // The dedicated EffectWindow property is added by the complete Deepin API
    // port; caption matching keeps this effect correct while that lands.
    if (w->caption().contains(QLatin1String("deepin-watermark"))) {
        effects->paintWindow(renderTarget, viewport, w, mask, region, data);
        return;
    }

    auto area = effects->clientArea(MaximizeFullArea, w);
    QRegion reg(area.toRect());
    if (m_maxiWin == w) {
        float t = m_animationTime.value();
        QRect r = m_newGeo.toRect();
        QRect r1 = m_oldGeo.toRect();
        QPoint p0 = r.topLeft();
        QPoint p1 = QPoint(r1.x(), r1.y());

        float w0 = r.width();
        float w1 = r1.width();
        float wt = w0 - (w0 - w1) * (1 - t);
        float k = wt / w0;

        float h0 = r.height();
        float h1 = r1.height();
        float ht = h0 - (h0 - h1) * (1 - t);
        float k1 = ht / h0;

        QPoint p = (p1 - p0) * (1 - t) + p0;
        data.setScale(QVector2D(k, k1));
        data += QPoint(qRound(p.x() - w->x()), qRound(p.y() - w->y()));
    }
    effects->paintWindow(renderTarget, viewport, w, mask, reg, data);
}

void MaximizeEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const QRegion &region, WindowPaintData &data)
{
    effects->drawWindow(renderTarget, viewport, w, mask, region, data);

    float t = m_animationTime.value();
    if (m_maxiWin != w || !m_texture || m_texture->isNull() || t > (effects->waylandDisplay() ? 0.2 : 0.7) || !m_isMaximized) {
        return;
    }

    QPoint p0 = QPoint(w->x(), w->y());
    QPoint p1 = m_oldGeo.toRect().topLeft();
    QPoint p = p1 - (p1 - p0) * t;

    float w0 = w->width();
    float w1 = m_oldGeo.width();
    float wt = (w0 - w1) * t + w1;

    float h0 = w->height();
    float h1 = m_oldGeo.height();
    float ht = (h0 - h1) * t + h1;

    m_texture->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    auto s = ShaderManager::instance()->pushShader(ShaderTrait::MapTexture);
    QMatrix4x4 mvp = viewport.projectionMatrix();
    mvp.translate(p.x(), p.y());
    s->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, mvp);
    m_texture->render(QSizeF(wt, ht));
    ShaderManager::instance()->popShader();
    m_texture->unbind();
    glDisable(GL_BLEND);
}

bool MaximizeEffect::supported()
{
    return effects->isOpenGLCompositing() && effects->animationsSupported();
}

bool MaximizeEffect::isActive() const
{
    return m_activated && !effects->isScreenLocked();
}

void MaximizeEffect::setActive(bool active)
{
    if (effects->activeFullScreenEffect() && effects->activeFullScreenEffect() != this) {
        return;
    }

    m_activated = active;

    if (active) {
        effects->setActiveFullScreenEffect(this);
    } else {
        cleanup();
    }

    effects->addRepaintFull();
}

void MaximizeEffect::cleanup()
{
    if (m_activated)
        return;

    m_maxiWin = nullptr;
    m_texture.reset();
    m_fbo.reset();
    effects->setActiveFullScreenEffect(nullptr);
}

void MaximizeEffect::slotWindowMaximizedStateChanged(EffectWindow *window, bool horizontal, bool vertical)
{
    if (window != m_maxiWin || m_oldGeo == window->frameGeometry() || horizontal != vertical) {
        setActive(false);
        return;
    }

    m_newGeo = window->frameGeometry();
    m_animationTime.reset();
    m_isMaximized = horizontal && vertical;
    setActive(true);
}

void MaximizeEffect::slotWindowMaximizedStateAboutToChange(EffectWindow *window, bool horizontal, bool vertical)
{
    Window *w = window ? window->window() : nullptr;
    if (!w || w->isInteractiveMove() || horizontal != vertical) {
        setActive(false);
        return;
    }

    m_oldGeo = window->frameGeometry();
    m_maxiWin = window;

    effects->makeOpenGLContextCurrent();

    const qreal scale = window->screen() ? window->screen()->scale() : 1.0;
    const QRectF geometry = w->frameGeometry();
    const QSize textureSize = (geometry.size() * scale).toSize();
    m_texture = GLTexture::allocate(GL_RGBA8, textureSize);
    if (!m_texture) {
        setActive(false);
        effects->doneOpenGLContextCurrent();
        return;
    }
    m_texture->setFilter(GL_LINEAR);
    m_texture->setWrapMode(GL_CLAMP_TO_EDGE);

    m_fbo = std::make_unique<GLFramebuffer>(m_texture.get());
    if (m_fbo->valid()) {
        RenderTarget renderTarget(m_fbo.get());
        RenderViewport viewport(geometry, scale, renderTarget);
        GLFramebuffer::pushFramebuffer(m_fbo.get());

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        WindowPaintData data;
        const int mask = PAINT_WINDOW_TRANSFORMED | PAINT_WINDOW_TRANSLUCENT;
        effects->drawWindow(renderTarget, viewport, window, mask, infiniteRegion(), data);

        GLFramebuffer::popFramebuffer();
    } else {
        setActive(false);
    }

    effects->doneOpenGLContextCurrent();
}

}

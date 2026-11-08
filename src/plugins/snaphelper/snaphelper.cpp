/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Lucas Murray <lmurray@undefinedfire.com>
    SPDX-FileCopyrightText: 2018 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "snaphelper.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"
#include "opengl/glutils.h"

#include <QPainter>

namespace KWin
{

static const int s_lineWidth = 4;
static const QColor s_lineColor = QColor(128, 128, 128, 128);

static QRegion computeDirtyRegion(const QRectF &windowRect)
{
    const QMargins outlineMargins(
        s_lineWidth / 2,
        s_lineWidth / 2,
        s_lineWidth / 2,
        s_lineWidth / 2);

    QRegion dirtyRegion;

    const QList<Output *> screens = effects->screens();
    for (Output *screen : screens) {
        const QRectF screenRect = effects->clientArea(ScreenArea, screen, effects->currentDesktop());

        QRectF screenWindowRect = windowRect;
        screenWindowRect.moveCenter(screenRect.center());

        QRectF verticalBarRect(0, 0, s_lineWidth, screenRect.height());
        verticalBarRect.moveCenter(screenRect.center());
        verticalBarRect.adjust(-1, -1, 1, 1);
        dirtyRegion += verticalBarRect.toAlignedRect();

        QRectF horizontalBarRect(0, 0, screenRect.width(), s_lineWidth);
        horizontalBarRect.moveCenter(screenRect.center());
        horizontalBarRect.adjust(-1, -1, 1, 1);
        dirtyRegion += horizontalBarRect.toAlignedRect();

        const QRectF outlineOuterRect = screenWindowRect
                                            .marginsAdded(outlineMargins)
                                            .adjusted(-1, -1, 1, 1);
        const QRectF outlineInnerRect = screenWindowRect
                                            .marginsRemoved(outlineMargins)
                                            .adjusted(1, 1, -1, -1);
        dirtyRegion += QRegion(outlineOuterRect.toRect()) - QRegion(outlineInnerRect.toRect());
    }

    return dirtyRegion;
}

SnapHelperEffect::SnapHelperEffect()
{
    reconfigure(ReconfigureAll);

    connect(effects, &EffectsHandler::windowClosed, this, &SnapHelperEffect::slotWindowClosed);
    connect(effects, &EffectsHandler::windowAdded, this, &SnapHelperEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowFrameGeometryChanged, this, &SnapHelperEffect::slotWindowFrameGeometryChanged);
    connect(effects, &EffectsHandler::windowDeleted, this, [this](EffectWindow *window) {
        if (window == m_window) {
            m_window = nullptr;
        }
    });
    for (EffectWindow *window : effects->stackingOrder()) {
        slotWindowAdded(window);
    }
}

SnapHelperEffect::~SnapHelperEffect()
{
}

void SnapHelperEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)
    m_animation.timeLine.setDuration(
        std::chrono::milliseconds(static_cast<int>(animationTime(std::chrono::milliseconds(250)))));
}

void SnapHelperEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    if (m_animation.active) {
        m_animation.timeLine.advance(presentTime);
    }

    effects->prePaintScreen(data, presentTime);
}

void SnapHelperEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, region, screen);
    if (!isActive() || !screen) {
        return;
    }

    const qreal opacityFactor = m_animation.active ? m_animation.timeLine.value() : 1.0;
    QColor color = s_lineColor;
    color.setAlphaF(color.alphaF() * opacityFactor);

    const QRectF workArea = effects->clientArea(ScreenArea, screen, effects->currentDesktop());
    QRectF centeredWindow = m_geometry;
    centeredWindow.moveCenter(workArea.center());
    const qreal halfWidth = s_lineWidth / 2.0;
    const QList<QRectF> guides{
        QRectF(workArea.center().x() - halfWidth, workArea.top(), s_lineWidth, workArea.height()),
        QRectF(workArea.left(), workArea.center().y() - halfWidth, workArea.width(), s_lineWidth),
        QRectF(centeredWindow.left() - halfWidth, centeredWindow.top() - halfWidth, centeredWindow.width() + s_lineWidth, s_lineWidth),
        QRectF(centeredWindow.right() - halfWidth, centeredWindow.top() + halfWidth, s_lineWidth, centeredWindow.height() - s_lineWidth),
        QRectF(centeredWindow.left() - halfWidth, centeredWindow.bottom() - halfWidth, centeredWindow.width() + s_lineWidth, s_lineWidth),
        QRectF(centeredWindow.left() - halfWidth, centeredWindow.top() + halfWidth, s_lineWidth, centeredWindow.height() - s_lineWidth),
    };

    if (effects->isOpenGLCompositing()) {
        GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
        vbo->reset();
        ShaderBinder binder(ShaderTrait::UniformColor | ShaderTrait::TransformColorspace);
        binder.shader()->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, viewport.projectionMatrix());
        binder.shader()->setColorspaceUniforms(ColorDescription::sRGB, renderTarget.colorDescription(), RenderingIntent::Perceptual);
        binder.shader()->setUniform(GLShader::ColorUniform::Color, color);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        QList<QVector2D> vertices;
        vertices.reserve(guides.size() * 6);
        const qreal scale = viewport.scale();
        for (const QRectF &guide : guides) {
            const QRectF r(guide.x() * scale, guide.y() * scale, guide.width() * scale, guide.height() * scale);
            vertices.append(QVector2D(r.left(), r.top()));
            vertices.append(QVector2D(r.right(), r.top()));
            vertices.append(QVector2D(r.left(), r.bottom()));
            vertices.append(QVector2D(r.left(), r.bottom()));
            vertices.append(QVector2D(r.right(), r.top()));
            vertices.append(QVector2D(r.right(), r.bottom()));
        }
        vbo->setVertices(vertices);
        vbo->render(GL_TRIANGLES);
        glDisable(GL_BLEND);
    } else if (effects->compositingType() == QPainterCompositing) {
        QPainter *painter = effects->scenePainter();
        painter->save();
        for (const QRectF &guide : guides) {
            painter->fillRect(guide, color);
        }
        painter->restore();
    }
}

void SnapHelperEffect::postPaintScreen()
{
    if (m_animation.active) {
        effects->addRepaint(computeDirtyRegion(m_geometry));
    }

    if (m_animation.timeLine.done()) {
        m_animation.active = false;
    }

    effects->postPaintScreen();
}

void SnapHelperEffect::slotWindowAdded(EffectWindow *w)
{
    connect(w, &EffectWindow::windowStartUserMovedResized, this, &SnapHelperEffect::slotWindowStartUserMovedResized);
    connect(w, &EffectWindow::windowFinishUserMovedResized, this, &SnapHelperEffect::slotWindowFinishUserMovedResized);
}

void SnapHelperEffect::slotWindowClosed(EffectWindow *w)
{
    if (w != m_window) {
        return;
    }

    m_window = nullptr;

    m_animation.active = true;
    m_animation.timeLine.setDirection(TimeLine::Backward);

    if (m_animation.timeLine.done()) {
        m_animation.timeLine.reset();
    }

    effects->addRepaint(computeDirtyRegion(m_geometry));
}

void SnapHelperEffect::slotWindowStartUserMovedResized(EffectWindow *w)
{
    if (!w->isMovable()) {
        return;
    }

    m_window = w;
    m_geometry = w->frameGeometry();

    m_animation.active = true;
    m_animation.timeLine.setDirection(TimeLine::Forward);

    if (m_animation.timeLine.done()) {
        m_animation.timeLine.reset();
    }

    effects->addRepaint(computeDirtyRegion(m_geometry));
}

void SnapHelperEffect::slotWindowFinishUserMovedResized(EffectWindow *w)
{
    if (w != m_window) {
        return;
    }

    m_window = nullptr;
    m_geometry = w->frameGeometry();

    m_animation.active = true;
    m_animation.timeLine.setDirection(TimeLine::Backward);

    if (m_animation.timeLine.done()) {
        m_animation.timeLine.reset();
    }

    effects->addRepaint(computeDirtyRegion(m_geometry));
}

void SnapHelperEffect::slotWindowFrameGeometryChanged(EffectWindow *w, const QRectF &old)
{
    if (w != m_window) {
        return;
    }

    m_geometry = w->frameGeometry();

    effects->addRepaint(computeDirtyRegion(old));
}

bool SnapHelperEffect::isActive() const
{
    return m_window != nullptr || m_animation.active;
}

} // namespace KWin

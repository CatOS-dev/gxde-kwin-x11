/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2012 Filip Wieladek <wattos@gmail.com>
    SPDX-FileCopyrightText: 2016 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "touchpoints.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glutils.h"

#include <QPainter>

#include <cmath>

namespace KWin
{

TouchPointsEffect::TouchPointsEffect()
    : Effect()
{
}

TouchPointsEffect::~TouchPointsEffect() = default;

static const Qt::GlobalColor s_colors[] = {
    Qt::blue,
    Qt::red,
    Qt::green,
    Qt::cyan,
    Qt::magenta,
    Qt::yellow,
    Qt::gray,
    Qt::darkBlue,
    Qt::darkRed,
    Qt::darkGreen};

Qt::GlobalColor TouchPointsEffect::colorForId(quint32 id)
{
    auto it = m_colors.constFind(id);
    if (it != m_colors.constEnd()) {
        return it.value();
    }
    static int s_colorIndex = -1;
    s_colorIndex = (s_colorIndex + 1) % 10;
    m_colors.insert(id, s_colors[s_colorIndex]);
    return s_colors[s_colorIndex];
}

bool TouchPointsEffect::touchDown(qint32 id, const QPointF &pos, std::chrono::microseconds time)
{
    TouchPoint point;
    point.pos = pos;
    point.press = true;
    point.color = colorForId(id);
    m_points << point;
    m_latestPositions.insert(id, pos);
    repaint();
    return false;
}

bool TouchPointsEffect::touchMotion(qint32 id, const QPointF &pos, std::chrono::microseconds time)
{
    TouchPoint point;
    point.pos = pos;
    point.press = true;
    point.color = colorForId(id);
    m_points << point;
    m_latestPositions.insert(id, pos);
    repaint();
    return false;
}

bool TouchPointsEffect::touchUp(qint32 id, std::chrono::microseconds time)
{
    auto it = m_latestPositions.constFind(id);
    if (it != m_latestPositions.constEnd()) {
        TouchPoint point;
        point.pos = it.value();
        point.press = false;
        point.color = colorForId(id);
        m_points << point;
    }
    m_latestPositions.remove(id);
    m_colors.remove(id);
    repaint();
    return false;
}

void TouchPointsEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    int time = 0;
    if (m_lastPresentTime.count()) {
        time = (presentTime - m_lastPresentTime).count();
    }

    auto it = m_points.begin();
    while (it != m_points.end()) {
        it->time += time;
        if (it->time > m_ringLife) {
            it = m_points.erase(it);
        } else {
            it++;
        }
    }

    if (m_points.isEmpty()) {
        m_lastPresentTime = std::chrono::milliseconds::zero();
    } else {
        m_lastPresentTime = presentTime;
    }

    effects->prePaintScreen(data, presentTime);
}

void TouchPointsEffect::paintScreen(const RenderTarget &renderTarget, const RenderViewport &viewport, int mask, const QRegion &region, Output *screen)
{
    effects->paintScreen(renderTarget, viewport, mask, region, screen);
    if (m_points.isEmpty()) {
        return;
    }

    if (effects->isOpenGLCompositing()) {
        ShaderBinder binder(ShaderTrait::UniformColor | ShaderTrait::TransformColorspace);
        binder.shader()->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, viewport.projectionMatrix());
        binder.shader()->setColorspaceUniforms(ColorDescription::sRGB, renderTarget.colorDescription(), RenderingIntent::Perceptual);
        glLineWidth(m_lineWidth);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (const TouchPoint &point : std::as_const(m_points)) {
            for (int ring = 0; ring < m_ringCount; ++ring) {
                const float alpha = computeAlpha(point.time, ring);
                const float size = computeRadius(point.time, point.press, ring);
                if (size > 0 && alpha > 0) {
                    QColor color = point.color;
                    color.setAlphaF(alpha);
                    drawCircleGl(color, point.pos.x(), point.pos.y(), size, viewport.scale(), binder.shader());
                }
            }
        }
        glDisable(GL_BLEND);
        glLineWidth(1.0);
    } else if (effects->compositingType() == QPainterCompositing) {
        for (const TouchPoint &point : std::as_const(m_points)) {
            for (int ring = 0; ring < m_ringCount; ++ring) {
                const float alpha = computeAlpha(point.time, ring);
                const float size = computeRadius(point.time, point.press, ring);
                if (size > 0 && alpha > 0) {
                    QColor color = point.color;
                    color.setAlphaF(alpha);
                    drawCircleQPainter(color, point.pos.x(), point.pos.y(), size);
                }
            }
        }
    }
}

void TouchPointsEffect::postPaintScreen()
{
    effects->postPaintScreen();
    repaint();
}

float TouchPointsEffect::computeRadius(int time, bool press, int ring)
{
    float ringDistance = m_ringLife / (m_ringCount * 3);
    if (press) {
        return ((time - ringDistance * ring) / m_ringLife) * m_ringMaxSize;
    }
    return ((m_ringLife - time - ringDistance * ring) / m_ringLife) * m_ringMaxSize;
}

float TouchPointsEffect::computeAlpha(int time, int ring)
{
    float ringDistance = m_ringLife / (m_ringCount * 3);
    return (m_ringLife - (float)time - ringDistance * (ring)) / m_ringLife;
}

void TouchPointsEffect::repaint()
{
    if (!m_points.isEmpty()) {
        QRegion dirtyRegion;
        const int radius = m_ringMaxSize + m_lineWidth;
        for (auto it = m_points.constBegin(), end = m_points.constEnd(); it != end; ++it) {
            dirtyRegion |= QRect(it->pos.x() - radius, it->pos.y() - radius, 2 * radius, 2 * radius);
        }
        effects->addRepaint(dirtyRegion);
    }
}

bool TouchPointsEffect::isActive() const
{
    return !m_points.isEmpty();
}

void TouchPointsEffect::drawCircleGl(const QColor &color, float cx, float cy, float radius, qreal scale, GLShader *shader)
{
    constexpr int segments = 80;
    constexpr float pi = 3.1415926f;
    QList<QVector2D> vertices;
    vertices.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        const float angle = 2 * pi * i / segments;
        vertices.append(QVector2D((cx + radius * std::cos(angle)) * scale,
                                  (cy + radius * std::sin(angle)) * scale));
    }

    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    shader->setUniform(GLShader::ColorUniform::Color, color);
    vbo->setVertices(vertices);
    vbo->render(GL_LINE_LOOP);
}

void TouchPointsEffect::drawCircleQPainter(const QColor &color, float cx, float cy, float radius)
{
    QPainter *painter = effects->scenePainter();
    painter->save();
    QPen pen(color);
    pen.setWidthF(m_lineWidth);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(cx, cy), radius, radius);
    painter->restore();
}

} // namespace

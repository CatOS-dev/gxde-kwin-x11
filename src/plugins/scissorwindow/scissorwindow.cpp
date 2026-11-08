// SPDX-FileCopyrightText: 2018-2023 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scissorwindow.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"
#include "kwineffectsex.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "opengl/gltexture.h"
#include "window.h"

#include <QImage>
#include <QPainter>

#include <algorithm>

Q_DECLARE_METATYPE(QPainterPath)

static void ensureResources()
{
    Q_INIT_RESOURCE(scissor);
}

namespace KWin
{

ScissorWindow::ScissorWindow()
{
    ensureResources();

    const ShaderTraits traits = ShaderTrait::MapTexture | ShaderTrait::Modulate | ShaderTrait::AdjustSaturation | ShaderTrait::TransformColorspace;
    m_maskShader = ShaderManager::instance()->generateShaderFromFile(traits, QString(), QStringLiteral(":/effects/scissor/mask.frag"));

    connect(effects, &EffectsHandler::windowDataChanged, this, [this](EffectWindow *window, int role) {
        if (role == WindowRadiusRole || role == WindowClipPathRole || role == WindowScissorForceRole) {
            updateWindow(window);
            window->addRepaintFull();
        }
    });
    connect(effects, &EffectsHandler::windowAdded, this, &ScissorWindow::updateWindow);
    connect(effects, &EffectsHandler::windowDeleted, this, [this](EffectWindow *window) {
        m_masks.erase(window);
        m_originalRadii.remove(window);
        m_redirectedWindows.remove(window);
    });

    const auto windows = effects->stackingOrder();
    for (EffectWindow *window : windows) {
        updateWindow(window);
    }
}

ScissorWindow::~ScissorWindow()
{
    const auto windows = m_originalRadii.keys();
    for (EffectWindow *window : windows) {
        restoreWindow(window);
    }
}

bool ScissorWindow::supported()
{
    return effects->isOpenGLCompositing() && OffscreenEffect::supported();
}

bool ScissorWindow::enabledByDefault()
{
    return supported();
}

int ScissorWindow::requestedEffectChainPosition() const
{
    return 99;
}

bool ScissorWindow::isActive() const
{
    return !m_originalRadii.isEmpty() || !m_redirectedWindows.isEmpty();
}

bool ScissorWindow::shouldScissor(EffectWindow *window) const
{
    if (window->isScissorForce()) {
        return true;
    }
    if (window->isDesktop() || window->isOutline() || window->isFullScreen()
        || effects->isSplitWin(window) || effects->hasActiveFullScreenEffect()) {
        return false;
    }
    if (window->window()->maximizeMode() == MaximizeFull) {
        return false;
    }
    return window->data(WindowRadiusRole).isValid() || window->data(WindowClipPathRole).isValid();
}

void ScissorWindow::updateWindow(EffectWindow *window)
{
    if (!window || !window->window() || !shouldScissor(window)) {
        restoreWindow(window);
        return;
    }

    if (!m_originalRadii.contains(window)) {
        m_originalRadii.insert(window, window->window()->borderRadius());
    }

    const QVariant clipPath = window->data(WindowClipPathRole);
    if (clipPath.isValid() && !qvariant_cast<QPainterPath>(clipPath).isEmpty()) {
        window->window()->setBorderRadius(BorderRadius());
        if (!m_redirectedWindows.contains(window)) {
            redirect(window);
            setShader(window, m_maskShader.get());
            m_redirectedWindows.insert(window);
        }
        return;
    }

    if (m_redirectedWindows.remove(window)) {
        unredirect(window);
        m_masks.erase(window);
    }

    QPointF radius;
    const QVariant radiusData = window->data(WindowRadiusRole);
    if (radiusData.isValid()) {
        radius = radiusData.toPointF();
    } else if (window->isScissorForce()) {
        const qreal systemRadius = effectsEx->getOsRadius() * effectsEx->getOsScale();
        radius = QPointF(systemRadius, systemRadius);
    }

    const qreal maximumRadius = std::min(window->width(), window->height()) / 2.0;
    const qreal clippedRadius = std::clamp(std::min(radius.x(), radius.y()), qreal(0), maximumRadius);
    window->window()->setBorderRadius(BorderRadius(clippedRadius));
}

void ScissorWindow::restoreWindow(EffectWindow *window)
{
    if (!window) {
        return;
    }
    if (m_redirectedWindows.remove(window)) {
        unredirect(window);
    }
    m_masks.erase(window);
    if (auto it = m_originalRadii.find(window); it != m_originalRadii.end()) {
        if (window->window()) {
            window->window()->setBorderRadius(it.value());
        }
        m_originalRadii.erase(it);
    }
}

GLTexture *ScissorWindow::maskTexture(EffectWindow *window, qreal scale)
{
    const QPainterPath path = qvariant_cast<QPainterPath>(window->data(WindowClipPathRole));
    const QRectF expandedGeometry = window->expandedGeometry();
    const QSize textureSize = (expandedGeometry.size() * scale).toSize();
    if (textureSize.isEmpty()) {
        return nullptr;
    }
    const QPointF frameOffset = window->frameGeometry().topLeft() - expandedGeometry.topLeft();
    MaskCache &cache = m_masks[window];

    if (cache.texture && cache.path == path && cache.textureSize == textureSize && cache.frameOffset == frameOffset && qFuzzyCompare(cache.scale, scale)) {
        return cache.texture.get();
    }

    QImage image(textureSize, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(scale, scale);
    painter.translate(frameOffset);
    painter.fillPath(path, Qt::white);
    painter.end();

    cache.path = path;
    cache.textureSize = textureSize;
    cache.frameOffset = frameOffset;
    cache.scale = scale;
    cache.texture = GLTexture::upload(image);
    if (!cache.texture) {
        return nullptr;
    }
    cache.texture->setFilter(GL_LINEAR);
    cache.texture->setWrapMode(GL_CLAMP_TO_EDGE);
    return cache.texture.get();
}

void ScissorWindow::prePaintWindow(EffectWindow *window, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
{
    updateWindow(window);
    effects->prePaintWindow(window, data, presentTime);
}

void ScissorWindow::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask, const QRegion &region, WindowPaintData &data)
{
    if (!m_redirectedWindows.contains(window)) {
        effects->drawWindow(renderTarget, viewport, window, mask, region, data);
        return;
    }

    GLTexture *texture = maskTexture(window, viewport.scale());
    if (!texture || texture->isNull()) {
        effects->drawWindow(renderTarget, viewport, window, mask, region, data);
        return;
    }

    {
        ShaderBinder binder(m_maskShader.get());
        m_maskShader->setUniform("maskTexture", 1);
    }
    glActiveTexture(GL_TEXTURE1);
    texture->bind();
    glActiveTexture(GL_TEXTURE0);

    OffscreenEffect::drawWindow(renderTarget, viewport, window, mask, region, data);

    glActiveTexture(GL_TEXTURE1);
    texture->unbind();
    glActiveTexture(GL_TEXTURE0);
}

} // namespace KWin

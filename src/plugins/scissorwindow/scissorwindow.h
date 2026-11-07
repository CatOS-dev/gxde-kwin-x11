// SPDX-FileCopyrightText: 2018-2023 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "effect/offscreeneffect.h"
#include "scene/borderradius.h"

#include <QHash>
#include <QPainterPath>

#include <map>

namespace KWin
{

class GLShader;
class GLTexture;

class ScissorWindow : public OffscreenEffect
{
    Q_OBJECT

public:
    ScissorWindow();
    ~ScissorWindow() override;

    static bool supported();
    static bool enabledByDefault();

    int requestedEffectChainPosition() const override;
    bool isActive() const override;

    void prePaintWindow(EffectWindow *window, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *window, int mask, const QRegion &region, WindowPaintData &data) override;

private:
    struct MaskCache
    {
        QPainterPath path;
        QSize textureSize;
        QPointF frameOffset;
        qreal scale = 1;
        std::unique_ptr<GLTexture> texture;
    };

    bool shouldScissor(EffectWindow *window) const;
    void updateWindow(EffectWindow *window);
    void restoreWindow(EffectWindow *window);
    GLTexture *maskTexture(EffectWindow *window, qreal scale);

    std::unique_ptr<GLShader> m_maskShader;
    QHash<EffectWindow *, BorderRadius> m_originalRadii;
    std::map<EffectWindow *, MaskCache> m_masks;
    QSet<EffectWindow *> m_redirectedWindows;
};

} // namespace KWin

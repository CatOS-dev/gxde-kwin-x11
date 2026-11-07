/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CHAMELEONBUTTON_H
#define CHAMELEONBUTTON_H

#include <KDecoration3/DecorationButton>
#include <QColor>
#include <QTimer>

namespace KWin
{
class EffectWindow;
}

class ChameleonButton : public KDecoration3::DecorationButton
{
    Q_OBJECT
public:
    explicit ChameleonButton(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent = nullptr);
    virtual ~ChameleonButton();

    static KDecoration3::DecorationButton *create(KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent);

    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

protected Q_SLOTS:
    void onCompositorChanged(bool);

protected:
    void paint(QPainter *painter, const QRectF &repaintRegion) override;
    KDecoration3::DecorationButtonType m_type;

    QTimer *max_hover_timer = nullptr;
    int m_mousePosX;

    QColor m_backgroundColor;
    KWin::EffectWindow *effect = nullptr;
    QTimer *max_timer = nullptr;
    bool m_isMaxAvailble = true;
};

#endif // CHAMELEONBUTTON_H

/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Deepin KWin contributors
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QPointer>
#include <QTimer>
#include <QWidget>

class QGraphicsDropShadowEffect;
class QHBoxLayout;
class QLabel;

namespace KWin
{

class Window;

class SplitMenu : public QWidget
{
    Q_OBJECT

public:
    static SplitMenu *instance();

    void showForWindow(const QRect &buttonRect, uint32_t windowId);
    void hideMenu(bool delayed, bool internal = false);
    void setKeepShowing(bool keep);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    SplitMenu();
    ~SplitMenu() override;

    void findWindow(uint32_t windowId);
    void updateTheme();
    void updatePosition(const QRect &buttonRect);
    void updateIcons();
    QPixmap iconPixmap(const QString &fileName, const QSize &size) const;
    bool isDarkTheme() const;
    void stopTimers();

    bool m_shown = false;
    bool m_keepShowing = false;
    bool m_entered = false;
    bool m_upsideDown = false;
    QPointer<Window> m_window;

    QColor m_backgroundColor;
    QColor m_borderColor;
    QHBoxLayout *m_layout;
    QLabel *m_leftLabel;
    QLabel *m_rightLabel;
    QLabel *m_maximizeLabel;
    QRect m_humpRect;
    QRect m_bodyRect;
    qreal m_scale = 1.0;

    QTimer m_showTimer;
    QTimer m_hideTimer;
    QGraphicsDropShadowEffect *m_shadow;
};

} // namespace KWin

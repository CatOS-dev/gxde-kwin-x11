/*
    KWin - the KDE window manager
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Deepin KWin contributors
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "splitmenu.h"

#include "window.h"
#include "workspace.h"
#if KWIN_BUILD_X11
#include "x11window.h"
#endif

#include <KLocalizedString>

#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QSvgRenderer>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace KWin
{

static constexpr int s_showDelay = 500;
static constexpr int s_hideDelay = 300;

SplitMenu *SplitMenu::instance()
{
    static SplitMenu menu;
    return &menu;
}

SplitMenu::SplitMenu()
    : m_layout(new QHBoxLayout)
    , m_leftLabel(new QLabel(this))
    , m_rightLabel(new QLabel(this))
    , m_maximizeLabel(new QLabel(this))
    , m_shadow(new QGraphicsDropShadowEffect(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowTitle(QStringLiteral("gxde-kwin_x11-splitmenu"));
    setProperty("__gxde_kwin_splitmenu", true);

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        m_scale = std::max<qreal>(1.0, screen->logicalDotsPerInch() / 96.0);
    }

    const int spacing = qRound(5 * m_scale);
    const QSize labelSize = QSize(37, 37) * m_scale;
    m_layout->setSpacing(spacing);
    m_layout->addWidget(m_leftLabel);
    m_layout->addWidget(m_rightLabel);
    m_layout->addWidget(m_maximizeLabel);
    setLayout(m_layout);

    for (QLabel *label : {m_leftLabel, m_rightLabel, m_maximizeLabel}) {
        label->setFixedSize(labelSize);
        label->installEventFilter(this);
    }

    m_shadow->setOffset(QPointF(0, 6) * m_scale);
    m_shadow->setColor(QColor(0, 0, 0, 38));
    m_shadow->setBlurRadius(20 * m_scale);
    setGraphicsEffect(m_shadow);

    m_humpRect.setSize(QSize(50, 20) * m_scale);
    m_humpRect.moveLeft(qRound(m_shadow->blurRadius()) + 4 * spacing + 3 * m_leftLabel->width() / 2);
    m_bodyRect.setSize(QSize(8 * spacing + 3 * m_leftLabel->width(), 4 * spacing + m_leftLabel->height()));
    m_bodyRect.moveLeft(qRound(m_shadow->blurRadius()));
    setFixedSize(m_bodyRect.width() + 2 * qRound(m_shadow->blurRadius()),
                 m_bodyRect.height() + m_humpRect.height() + qRound(m_shadow->blurRadius()));

    m_showTimer.setSingleShot(true);
    connect(&m_showTimer, &QTimer::timeout, this, [this]() {
        m_shown = true;
        show();
        raise();
    });
    m_hideTimer.setSingleShot(true);
    connect(&m_hideTimer, &QTimer::timeout, this, [this]() {
        m_shown = false;
        m_entered = false;
        m_window.clear();
        hide();
    });
}

SplitMenu::~SplitMenu() = default;

void SplitMenu::showForWindow(const QRect &buttonRect, uint32_t windowId)
{
    if (m_shown) {
        return;
    }
    findWindow(windowId);
    if (!m_window) {
        return;
    }

    stopTimers();
    updateTheme();
    updatePosition(buttonRect);
    m_showTimer.start(s_showDelay);
}

void SplitMenu::hideMenu(bool delayed, bool internal)
{
    if ((m_entered && !internal) || m_keepShowing) {
        return;
    }
    stopTimers();
    m_hideTimer.start(!delayed || !m_shown ? 0 : s_hideDelay);
}

void SplitMenu::setKeepShowing(bool keep)
{
    m_keepShowing = keep;
}

void SplitMenu::enterEvent(QEnterEvent *event)
{
    m_entered = true;
    stopTimers();
    QWidget::enterEvent(event);
}

void SplitMenu::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    hideMenu(true, true);
}

bool SplitMenu::eventFilter(QObject *watched, QEvent *event)
{
    QLabel *label = qobject_cast<QLabel *>(watched);
    if (!label || (label != m_leftLabel && label != m_rightLabel && label != m_maximizeLabel)) {
        return QWidget::eventFilter(watched, event);
    }

    const bool hovered = event->type() == QEvent::Enter;
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        QString toolTip;
        if (label == m_leftLabel) {
            toolTip = i18n("Tile window to left of screen");
        } else if (label == m_rightLabel) {
            toolTip = i18n("Tile window to right of screen");
        } else {
            toolTip = m_window && m_window->maximizeMode() == MaximizeFull ? i18n("Unmaximize") : i18n("Maximize");
        }
        updateIcons();
        if (hovered) {
            const QPoint globalPoint = label->mapToGlobal(QPoint(label->width() / 2, label->height()));
            QToolTip::showText(globalPoint, toolTip, this);
        } else {
            QToolTip::hideText();
        }
        const QString base = label == m_leftLabel ? QStringLiteral("left_split_")
            : label == m_rightLabel               ? QStringLiteral("right_split_")
                                                  : (m_window && m_window->maximizeMode() == MaximizeFull ? QStringLiteral("restore_split_") : QStringLiteral("max_split_"));
        label->setPixmap(iconPixmap(base + (hovered ? QStringLiteral("hover.svg") : QStringLiteral("normal.svg")), label->size()));
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease && m_window) {
        if (label == m_leftLabel) {
            m_window->setQuickTileFromMenu(QuickTileFlag::Left, true);
        } else if (label == m_rightLabel) {
            m_window->setQuickTileFromMenu(QuickTileFlag::Right, true);
        } else if (m_window->maximizeMode() == MaximizeFull) {
            m_window->maximize(MaximizeRestore);
        } else {
            m_window->maximize(MaximizeFull);
        }
        hideMenu(false, true);
    }
    return false;
}

void SplitMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(m_borderColor, 1));

    const qreal base = m_upsideDown ? m_humpRect.top() : m_humpRect.bottom();
    const qreal sign = m_upsideDown ? -1 : 1;
    QPainterPath humpPath;
    humpPath.moveTo(m_humpRect.left(), base);
    const qreal step = m_humpRect.width() / 50.0;
    const qreal frequency = 2 * std::numbers::pi / m_humpRect.width();
    const qreal halfHeight = m_humpRect.height() / 2.0;
    for (qreal x = step; x < m_humpRect.width(); x += step) {
        const qreal y = halfHeight * std::sin(frequency * x - std::numbers::pi / 2) + halfHeight;
        humpPath.lineTo(m_humpRect.left() + x, base - sign * y);
    }
    humpPath.lineTo(m_humpRect.right(), base);

    QPainterPath bodyPath;
    bodyPath.addRoundedRect(m_bodyRect, 14 * m_scale, 14 * m_scale);
    painter.drawPath(humpPath);
    painter.drawPath(bodyPath);
    painter.fillPath(humpPath, m_backgroundColor);
    painter.fillPath(bodyPath, m_backgroundColor);
}

void SplitMenu::findWindow(uint32_t windowId)
{
    m_window.clear();
#if KWIN_BUILD_X11
    m_window = workspace()->findClient(Predicate::WindowMatch, windowId);
#else
    Q_UNUSED(windowId)
#endif
}

bool SplitMenu::isDarkTheme() const
{
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

void SplitMenu::updateTheme()
{
    if (isDarkTheme()) {
        m_backgroundColor = QColor(0, 0, 0);
        m_borderColor = QColor(55, 55, 55, 179);
    } else {
        m_backgroundColor = QColor(255, 255, 255);
        m_borderColor = QColor(185, 185, 185, 179);
    }
    updateIcons();
}

void SplitMenu::updateIcons()
{
    m_leftLabel->setPixmap(iconPixmap(QStringLiteral("left_split_normal.svg"), m_leftLabel->size()));
    m_rightLabel->setPixmap(iconPixmap(QStringLiteral("right_split_normal.svg"), m_rightLabel->size()));
    const QString maximizeIcon = m_window && m_window->maximizeMode() == MaximizeFull
        ? QStringLiteral("restore_split_normal.svg")
        : QStringLiteral("max_split_normal.svg");
    m_maximizeLabel->setPixmap(iconPixmap(maximizeIcon, m_maximizeLabel->size()));
}

void SplitMenu::updatePosition(const QRect &buttonRect)
{
    const QRect area = workspace()->clientArea(MaximizeArea, m_window, buttonRect.center()).toRect();
    QPoint menuPosition(buttonRect.center().x() - m_humpRect.center().x(), buttonRect.bottom() + 5);

    m_upsideDown = false;
    QMargins margins(qRound(m_shadow->blurRadius()), m_humpRect.height(), qRound(m_shadow->blurRadius()), qRound(m_shadow->blurRadius()));
    m_humpRect.moveTop(1);
    m_bodyRect.moveTop(m_humpRect.bottom());
    const int maximumX = std::max(area.left(), area.right() - width());
    menuPosition.setX(std::clamp(menuPosition.x(), area.left(), maximumX));
    if (menuPosition.y() + height() > area.bottom()) {
        m_upsideDown = true;
        margins = QMargins(qRound(m_shadow->blurRadius()), qRound(m_shadow->blurRadius()), qRound(m_shadow->blurRadius()), m_humpRect.height());
        m_bodyRect.moveTop(qRound(m_shadow->blurRadius()));
        m_humpRect.moveTop(m_bodyRect.bottom());
        menuPosition.setY(buttonRect.top() - height() + 1);
    }
    menuPosition.setY(std::max(menuPosition.y(), area.top()));

    m_layout->setContentsMargins(margins);
    move(menuPosition);
}

QPixmap SplitMenu::iconPixmap(const QString &fileName, const QSize &size) const
{
    const QString theme = isDarkTheme() ? QStringLiteral("dark") : QStringLiteral("light");
    const QString path = QStringLiteral(":/gxde-kwin_x11/splitmenu/themes/%1/icons/%2").arg(theme, fileName);
    QSvgRenderer renderer(path);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    return pixmap;
}

void SplitMenu::stopTimers()
{
    m_showTimer.stop();
    m_hideTimer.stop();
}

} // namespace KWin

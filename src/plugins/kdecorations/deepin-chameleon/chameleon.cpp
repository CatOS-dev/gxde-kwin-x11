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
#include "chameleon.h"
#include "chameleonbutton.h"
#include "chameleonconfig.h"
#include "chameleonshadow.h"
#include "chameleontheme.h"
#include "chameleonwindowtheme.h"
#include "kwinutils.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationButtonGroup>
#include <KDecoration3/DecorationSettings>

#include <QDebug>
#include <QObject>
#include <QPainter>

#include <algorithm>

#include "effect/effectwindow.h"
#include "window.h"
#include "windowstyle/decorationstyle.h"
#include "workspace.h"
#if KWIN_BUILD_X11
#include "x11window.h"
#endif

Q_DECLARE_METATYPE(QPainterPath)

Chameleon::Chameleon(QObject *parent, const QVariantList &args)
    : KDecoration3::Decoration(parent, args)
    , m_window(qobject_cast<KWin::Window *>(parent))
{
}

Chameleon::~Chameleon()
{
}

bool Chameleon::init()
{
    if (m_initialized) {
        return true;
    }
    if (!m_window || !window()) {
        return false;
    }

    auto c = window();

    initButtons();

    // 要放到updateTheme调用之前初始化此对象
    auto global_config = ChameleonConfig::instance();
    m_theme = new ChameleonWindowTheme(m_window, this);

    m_font = settings()->font();
    updateTheme();

    connect(global_config, &ChameleonConfig::themeChanged, this, &Chameleon::updateTheme);
    connect(settings().get(), &KDecoration3::DecorationSettings::alphaChannelSupportedChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration3::DecoratedWindow::activeChanged, this, &Chameleon::updateConfig);
    connect(c, &KDecoration3::DecoratedWindow::widthChanged, this, &Chameleon::onClientWidthChanged);
    connect(c, &KDecoration3::DecoratedWindow::heightChanged, this, &Chameleon::onClientHeightChanged);
    connect(c, &KDecoration3::DecoratedWindow::maximizedChanged, this, &Chameleon::updateTitleBarArea);
    connect(c, &KDecoration3::DecoratedWindow::adjacentScreenEdgesChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration3::DecoratedWindow::maximizedHorizontallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration3::DecoratedWindow::maximizedVerticallyChanged, this, &Chameleon::updateBorderPath);
    connect(c, &KDecoration3::DecoratedWindow::captionChanged, this, &Chameleon::updateTitleGeometry);
    connect(c, &KDecoration3::DecoratedWindow::maximizeableChanged, this, &Chameleon::updateTitleBarArea);
    connect(this, &Chameleon::noTitleBarChanged, this, &Chameleon::updateTitleBarArea, Qt::QueuedConnection);
    connect(m_theme, &ChameleonWindowTheme::themeChanged, this, &Chameleon::updateTheme);
    connect(m_theme, &ChameleonWindowTheme::windowRadiusChanged, this, &Chameleon::updateBorderPath);
    connect(m_theme, &ChameleonWindowTheme::windowRadiusChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::borderWidthChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::borderColorChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowRadiusChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowOffectChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::shadowColorChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::mouseInputAreaMarginsChanged, this, &Chameleon::updateMouseInputAreaMargins);
    connect(m_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, &Chameleon::updateShadow);
    connect(m_theme, &ChameleonWindowTheme::windowPixelRatioChanged, this, &Chameleon::updateTitleBarArea);
    connect(settings().get(), &KDecoration3::DecorationSettings::fontChanged, this, [this](const QFont &font) {
        m_font = font;
        updateTitleGeometry();
    });
    connect(KWin::Workspace::self(), &KWin::Workspace::osRadiusChanged, this, &Chameleon::updateBorderPath);
    connect(KWin::Workspace::self(), &KWin::Workspace::osRadiusChanged, this, &Chameleon::updateShadow);
    connect(ChameleonConfig::instance(), &ChameleonConfig::titlebarHeightChanged, this, &Chameleon::handleTitlebarHeightChanged);
    if (m_window->windowStyleObj()) {
        connect(m_window->windowStyleObj(), &KWin::DecorationStyle::noTitleBarChanged, this, &Chameleon::updateTitleBarArea);
        connect(m_window->windowStyleObj(), &KWin::DecorationStyle::windowEffectChanged, this, &Chameleon::updateTitleBarArea);
        connect(m_window->windowStyleObj(), &KWin::DecorationStyle::windowEffectChanged, this, &Chameleon::updateShadow);
    }

    m_initialized = true;
    return true;
}

void Chameleon::handleTitlebarHeightChanged()
{
    updateTitleBarArea();
}

void Chameleon::paint(QPainter *painter, const QRectF &repaintArea)
{
    painter->setClipPath(m_borderPath);

    if (windowNeedBorder()) {
        qreal border_width = borderWidth();

        // 支持alpha通道时在阴影上绘制border
        if (!qIsNull(border_width)) {
            painter->fillPath(m_borderPath, borderColor()); // m_borderPath
            // painter->fillRect(repaintArea, borderColor());//m_borderPath
        }
    }

    if (!noTitleBar()) {
        painter->setFont(m_font);

        painter->fillRect(titleBar() & repaintArea, getBackgroundColor());
        painter->setPen(getTextColor());
        painter->drawText(m_titleArea, Qt::AlignCenter, m_title);

        // draw all buttons
        m_leftButtons->paint(painter, repaintArea);
        m_rightButtons->paint(painter, repaintArea);
    }
}

const ChameleonTheme::ThemeConfig *Chameleon::themeConfig() const
{
    return m_config;
}

KWin::EffectWindow *Chameleon::effect() const
{
    if (m_effect) {
        return m_effect.data();
    }

    if (!m_window) {
        return nullptr;
    }

    Chameleon *self = const_cast<Chameleon *>(this);
    self->m_effect = m_window->effectWindow();
    Q_EMIT self->effectInitialized(m_effect.data());

    return m_effect.data();
}

KWin::Window *Chameleon::kwinWindow() const
{
    return m_window;
}

uint32_t Chameleon::windowId() const
{
#if KWIN_BUILD_X11
    if (const auto x11Window = qobject_cast<KWin::X11Window *>(m_window)) {
        return x11Window->window();
    }
#endif
    return effect() ? static_cast<uint32_t>(effect()->windowId()) : 0;
}

bool Chameleon::noTitleBar() const
{
    return m_window && m_window->windowStyleObj() && m_window->windowStyleObj()->noTitleBar();
}

qreal Chameleon::borderWidth() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderWidthProperty)) {
        return m_theme->borderWidth();
    }

    return m_config->borderConfig.borderWidth;
}

qreal Chameleon::titleBarHeight() const
{
    return ChameleonConfig::m_titlebarHeight * m_theme->windowPixelRatio();
}

qreal Chameleon::shadowRadius() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowRadiusProperty)) {
        return m_theme->shadowRadius();
    }

    return m_config->shadowConfig.shadowRadius;
}

QPointF Chameleon::shadowOffset() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowOffsetProperty)) {
        return m_theme->shadowOffset();
    }

    return m_config->shadowConfig.shadowOffset;
}

QPointF Chameleon::windowRadius() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
        return m_theme->windowRadius();
    }

    return m_config->radius;
}

QMarginsF Chameleon::mouseInputAreaMargins() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::MouseInputAreaMargins)) {
        return m_theme->mouseInputAreaMargins();
    }

    return m_config->mouseInputAreaMargins;
}

QColor Chameleon::shadowColor() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowColorProperty)) {
        return m_theme->shadowColor();
    }

    return m_config->shadowConfig.shadowColor;
}

QColor Chameleon::borderColor() const
{
    if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderColorProperty)) {
        return m_theme->borderColor();
    }

    return m_config->borderConfig.borderColor;
}

QIcon Chameleon::menuIcon() const
{
    return m_config->titlebarConfig.menuBtn.btnIcon;
}

QIcon Chameleon::minimizeIcon() const
{
    return m_config->titlebarConfig.minimizeBtn.btnIcon;
}

QIcon Chameleon::maximizeIcon() const
{
    return m_config->titlebarConfig.maximizeBtn.btnIcon;
}

QIcon Chameleon::unmaximizeIcon() const
{
    return m_config->titlebarConfig.unmaximizeBtn.btnIcon;
}

QIcon Chameleon::closeIcon() const
{
    return m_config->titlebarConfig.closeBtn.btnIcon;
}

QPointF Chameleon::menuIconPos() const
{
    return m_config->titlebarConfig.menuBtn.pos;
}

qint32 Chameleon::menuIconWidth() const
{
    return m_config->titlebarConfig.menuBtn.width;
}

qint32 Chameleon::menuIconHeight() const
{
    return m_config->titlebarConfig.menuBtn.height;
}

void Chameleon::initButtons()
{
    m_leftButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Left, this, &ChameleonButton::create);
    m_rightButtons = new KDecoration3::DecorationButtonGroup(KDecoration3::DecorationButtonGroup::Position::Right, this, &ChameleonButton::create);
    connect(m_rightButtons, &KDecoration3::DecorationButtonGroup::geometryChanged,
            this, &Chameleon::updateTitleBarArea, Qt::QueuedConnection);
}

void Chameleon::updateButtonsGeometry()
{
    auto s = settings();
    auto c = window();

    // adjust button position
    const int bHeight = noTitleBar() ? 0 : titleBarHeight();
    const int bWidth = 50 * m_theme->windowPixelRatio();

    for (KDecoration3::DecorationButton *button : m_leftButtons->buttons() + m_rightButtons->buttons()) {
        button->setGeometry(QRectF(QPoint(0, 0), QSizeF(bWidth, bHeight)));
    }

    // left buttons
    if (!m_leftButtons->buttons().isEmpty()) {
        // spacing
        m_leftButtons->setSpacing(0);

        // padding
        const int vPadding = noTitleBar() ? 0 : (titleBarHeight() - bHeight) / 2;
        const int hPadding = s->smallSpacing();

        m_leftButtons->buttons().front()->setGeometry(QRectF(menuIconPos(), QSizeF(menuIconWidth(), menuIconHeight())));
        if (c->isMaximizedHorizontally()) {
            // add offsets on the side buttons, to preserve padding, but satisfy Fitts law
            m_leftButtons->setPos(QPointF(menuIconPos().x(), menuIconPos().y() + vPadding));
        } else {
            m_leftButtons->setPos(QPointF(menuIconPos().x() + hPadding + borderLeft(), menuIconPos().y() + vPadding));
        }
    }

    // right buttons
    if (!m_rightButtons->buttons().isEmpty()) {
        // spacing
        m_rightButtons->setSpacing(0);

        // padding
        const int vPadding = noTitleBar() ? 0 : -(bHeight - titleBarHeight()) / 2;
        const int hPadding = 0;

        if (c->isMaximizedHorizontally()) {
            m_rightButtons->buttons().back()->setGeometry(QRectF(QPoint(0, 0), QSizeF(bWidth + hPadding, bHeight)));
            m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width(), vPadding));
        } else {
            m_rightButtons->setPos(QPointF(size().width() - m_rightButtons->geometry().width() - hPadding - borderRight(), vPadding));
        }
    }

    updateTitleGeometry();
}

void Chameleon::updateTitleGeometry()
{
    auto s = settings();

    m_titleArea = titleBar();

    m_title = window()->caption();
    // 使用系统字体，不要使用 settings() 中的字体
    const QFontMetricsF fontMetrics(m_font);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    int full_width = fontMetrics.width(m_title) * m_theme->windowPixelRatio();
#else
    int full_width = fontMetrics.horizontalAdvance(m_title) * m_theme->windowPixelRatio();
#endif

    if (m_config->titlebarConfig.area == Qt::TopEdge || m_config->titlebarConfig.area == Qt::BottomEdge) {
        int buttons_width = m_leftButtons->geometry().width()
            + m_rightButtons->geometry().width() + 2 * s->smallSpacing();

        m_titleArea.setWidth(m_titleArea.width() - buttons_width);
        m_titleArea.moveLeft(m_leftButtons->geometry().right() + s->smallSpacing());

        if (full_width < (m_titleArea.right() - titleBar().center().x()) * 2) {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveCenter(titleBar().center());

        } else if (full_width > m_titleArea.width()) {
            m_title = fontMetrics.elidedText(m_title,
                                             Qt::ElideRight, qMax(m_titleArea.width(), m_titleArea.height()));
            m_titleArea.moveRight(m_rightButtons->geometry().left() + s->smallSpacing());

        } else {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveRight(m_rightButtons->geometry().left() + s->smallSpacing());
        }

    } else {
        int buttons_height = m_leftButtons->geometry().height()
            + m_rightButtons->geometry().height() + 2 * s->smallSpacing();

        m_titleArea.setHeight(m_titleArea.height() - buttons_height);
        m_titleArea.moveTop(m_leftButtons->geometry().bottom() + s->smallSpacing());

        if (full_width < (m_titleArea.bottom() - titleBar().center().y()) * 2) {
            m_titleArea.setHeight(full_width);
            m_titleArea.moveCenter(titleBar().center());

        } else if (full_width > m_titleArea.height()) {
            m_title = fontMetrics.elidedText(m_title,
                                             Qt::ElideRight, qMax(m_titleArea.width(), m_titleArea.height()));
            m_titleArea.moveBottom(m_rightButtons->geometry().top() + s->smallSpacing());

        } else {
            m_titleArea.setWidth(full_width);
            m_titleArea.moveBottom(m_rightButtons->geometry().top() + s->smallSpacing());
        }
    }

    update();
}

void Chameleon::updateTheme()
{
    QString theme_name;

    if (m_theme->propertyIsValid(ChameleonWindowTheme::ThemeProperty)) {
        theme_name = m_theme->theme();
    }

    KWin::Window *client = dynamic_cast<KWin::Window *>(m_window);
    if (!client) {
        qCCritical(CHAMELEON) << "The Window corresponding to chameleon is nullptr";
    }

    qCDebug(CHAMELEON) << "windowId: " << QString("0x%1").arg(client->property("windowId").toULongLong(), 0, 16) << " windowType: " << client->windowType();

    ChameleonTheme::ConfigGroup *configGroup;

    if (!theme_name.isEmpty()) {
        ChameleonTheme::instance()->loadTheme(theme_name);
    }

    configGroup = ChameleonTheme::instance()->themeConfig(client->windowType());

    if (m_configGroup == configGroup) {
        return;
    }

    m_configGroup = configGroup;
    updateConfig();
}

void Chameleon::updateConfig()
{
    auto c = window();

    bool active = c->isActive();
    m_config = active ? &m_configGroup->normal : &m_configGroup->inactive;

    updateMouseInputAreaMargins();
    updateTitleBarArea();
    updateShadow();
    update();
}

void Chameleon::updateTitleBarArea()
{
    auto c = window();

    m_titleBarAreaMargins.setLeft(0);
    m_titleBarAreaMargins.setTop(0);
    m_titleBarAreaMargins.setRight(0);
    m_titleBarAreaMargins.setBottom(0);

    qreal border_width = windowNeedBorder() ? borderWidth() : 0;
    qreal titlebar_height = noTitleBar() ? 0 : titleBarHeight();

    switch (m_config->titlebarConfig.area) {
    case Qt::LeftEdge:
        m_titleBarAreaMargins.setLeft(titlebar_height);
        setTitleBar(QRect(border_width, border_width, titlebar_height, c->height()));
        setBorders(QMargins(border_width + titlebar_height, border_width,
                            border_width, border_width));
        break;
    case Qt::RightEdge:
        m_titleBarAreaMargins.setRight(titlebar_height);
        setTitleBar(QRect(border_width + c->width() - titlebar_height, border_width,
                          titlebar_height, c->height()));
        setBorders(QMargins(border_width, border_width,
                            border_width + titlebar_height, border_width));
        break;
    case Qt::TopEdge:
        m_titleBarAreaMargins.setTop(titlebar_height);
        setTitleBar(QRect(border_width, border_width,
                          c->width(), titlebar_height));
        setBorders(QMargins(border_width, border_width + titlebar_height,
                            border_width, border_width));
        break;
    case Qt::BottomEdge:
        m_titleBarAreaMargins.setBottom(titlebar_height);
        setTitleBar(QRect(border_width, border_width + c->height() - titlebar_height,
                          c->width(), titlebar_height));
        setBorders(QMargins(border_width, border_width,
                            border_width, border_width + titlebar_height));
        break;
    default:
        return;
    }

    updateBorderPath();
    updateButtonsGeometry();
}

void Chameleon::updateBorderPath()
{
    auto c = window();
    QRectF client_rect(0, 0, c->width(), c->height());
    client_rect += borders();
    client_rect.moveTopLeft(QPointF(0, 0));

    QPainterPath path;
    KWin::EffectWindow *effect = this->effect();

    if (windowNeedRadius()) {
        QPointF window_radius = windowRadius();
        if (window_radius.isNull()) {
            const qreal radius = KWin::Workspace::self()->getWindowRadius() * KWin::Workspace::self()->getOsScreenScale();
            window_radius = QPointF(radius, radius);
        }
        window_radius.setX(std::clamp(window_radius.x(), 0.0, client_rect.width() / 2.0));
        window_radius.setY(std::clamp(window_radius.y(), 0.0, client_rect.height() / 2.0));
        path.addRoundedRect(client_rect, window_radius.x(), window_radius.y());

        if (effect && effect->data(ChameleonConfig::WindowRadiusRole).toPointF() != window_radius) {
            effect->setData(ChameleonConfig::WindowRadiusRole, window_radius);
        }
    } else {
        path.addRect(client_rect);
    }

    m_borderPath = path;

    update();
}

void Chameleon::updateShadow()
{
    KWin::EffectWindow *effectWindow = effect();
    if (!effectWindow || !m_config) {
        qCDebug(CHAMELEON) << " has not effect window";
        return;
    }

    if (const auto style = m_window->windowStyleObj(); style && style->isCancelShadow()) {
        effectWindow->setData(ChameleonConfig::ShadowMaskRole, QVariant());
        effectWindow->setData(ChameleonConfig::ShadowOffsetRole, QVariant());
        setShadow({});
        return;
    }

    ChameleonTheme::ThemeConfig effectiveConfig = *m_config;
    const QVariant windowRadiusData = effectWindow->data(ChameleonConfig::WindowRadiusRole);
    effectiveConfig.radius = windowRadiusData.isValid() ? windowRadiusData.toPointF() : QPointF();

    if (settings()->isAlphaChannelSupported()) {
        if (m_theme->validProperties() == ChameleonWindowTheme::PropertyFlags()) {
            const QVariant data_clip_path = effectWindow->data(ChameleonConfig::WindowClipPathRole);
            if (data_clip_path.isValid()) {
                return;
            }

            { /* set shadow image for scissor effect */
                auto shadow1 = ChameleonShadow::instance()->getShadow(&effectiveConfig, m_theme->windowPixelRatio());
                effectWindow->setData(ChameleonConfig::ShadowMaskRole, QVariant(shadow1->shadow()));
                effectWindow->setData(ChameleonConfig::ShadowOffsetRole, QVariant(shadow1->paddingTop()));
            }
            return setShadow(ChameleonShadow::instance()->getShadow(&effectiveConfig, m_theme->windowPixelRatio()));
        }

        qreal scale = m_theme->windowPixelRatio();
        // 优先使用窗口自己设置的属性
        // if (m_theme->propertyIsValid(ChameleonWindowTheme::WindowRadiusProperty)) {
        //     m_config->radius = m_theme->windowRadius();
        //     // 这里的数据是已经缩放过的，因此scale值需要为1
        //     scale = 1.0;
        // }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderWidthProperty)) {
            effectiveConfig.borderConfig.borderWidth = m_theme->borderWidth();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::BorderColorProperty)) {
            effectiveConfig.borderConfig.borderColor = m_theme->borderColor();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowRadiusProperty)) {
            effectiveConfig.shadowConfig.shadowRadius = m_theme->shadowRadius();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowOffsetProperty)) {
            effectiveConfig.shadowConfig.shadowOffset = m_theme->shadowOffset();
        }

        if (m_theme->propertyIsValid(ChameleonWindowTheme::ShadowColorProperty)) {
            effectiveConfig.shadowConfig.shadowColor = m_theme->shadowColor();
        }

        const QVariant data_clip_path = effectWindow->data(ChameleonConfig::WindowClipPathRole);
        if (data_clip_path.isValid()) {
            return;
        }

        auto shadow = ChameleonShadow::instance()->getShadow(&effectiveConfig, scale);
        setShadow(shadow);

        { /* set shadow image for scissor effect */
            effectWindow->setData(ChameleonConfig::ShadowMaskRole, QVariant(shadow->shadow()));
            effectWindow->setData(ChameleonConfig::ShadowOffsetRole, QVariant(shadow->paddingTop()));
        }
    }
}

void Chameleon::updateMouseInputAreaMargins()
{
    setResizeOnlyBorders(mouseInputAreaMargins().toMargins());
}

void Chameleon::onClientWidthChanged()
{
    updateTitleBarArea();
}

void Chameleon::onClientHeightChanged()
{
    updateTitleBarArea();
}

bool Chameleon::windowNeedRadius() const
{
    if (const auto style = m_window->windowStyleObj(); style && style->isCancelRadius()) {
        return false;
    }

    KWin::EffectWindow *effect = this->effect();
    if (effect && effect->isScissorForce()) {
        return true;
    }

    auto c = window();
    return KWinUtils::isCompositing() && c->adjacentScreenEdges() == Qt::Edges();
}

bool Chameleon::windowNeedBorder() const
{
    if (const auto style = m_window->windowStyleObj();
        style && style->effectScenes().testFlag(KWin::DecorationStyle::EffectNoBorder)) {
        return false;
    }

    if (window()->isMaximized()) {
        return false;
    }

    // 开启窗口混成时可以在阴影图片中绘制窗口边框
    if (settings()->isAlphaChannelSupported() && KWinUtils::isCompositing()) {
        return false;
    }

    return true;
}

QColor Chameleon::getTextColor() const
{
    if (m_config->titlebarConfig.font.textColor.isValid())
        return m_config->titlebarConfig.font.textColor;

    auto c = window();

    return c->color(c->isActive() ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive, KDecoration3::ColorRole::Foreground);
}

QColor Chameleon::getBackgroundColor() const
{
    if (m_config->titlebarConfig.backgroundColor.isValid())
        return m_config->titlebarConfig.backgroundColor;

    auto c = window();

    return c->color(c->isActive() ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive, KDecoration3::ColorRole::TitleBar);
}

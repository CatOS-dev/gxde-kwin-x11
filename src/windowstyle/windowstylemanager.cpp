/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "windowstylemanager.h"

#include "compositor.h"
#include "window.h"
#include "windowstyle/decorationstyle.h"
#include "workspace.h"

#include <QColor>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QScreen>

#include <algorithm>

namespace KWin
{

static constexpr auto s_appearanceInterface = "org.freedesktop.DBus.Properties";

WindowStyleManager::WindowStyleManager(QObject *parent)
    : QObject(parent)
{
    m_darkTheme = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
    m_activeColor = QGuiApplication::palette().color(QPalette::Highlight).name();
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        m_scale = std::clamp(screen->logicalDotsPerInch() / 96.0, qreal(1), qreal(8));
        connect(screen, &QScreen::logicalDotsPerInchChanged, this, [this](qreal dpi) {
            const qreal scale = std::clamp(dpi / 96.0, qreal(1), qreal(8));
            if (qFuzzyCompare(m_scale, scale)) {
                return;
            }
            m_scale = scale;
            Q_EMIT scaleChanged(scale);
            refreshAllWindows();
        });
    }

    QCoreApplication::instance()->installEventFilter(this);
    connect(workspace(), &Workspace::windowAdded, this, &WindowStyleManager::onWindowAdded);
    if (Compositor *compositor = Compositor::self()) {
        connect(compositor, &Compositor::compositingToggled, this, [this](bool active) {
            if (active) {
                for (Window *window : workspace()->windows()) {
                    window->updateWindowRadius(true);
                }
            }
        });
    }

    readAppearanceService(QStringLiteral("org.deepin.dde.Appearance1"),
                          QStringLiteral("/org/deepin/dde/Appearance1"),
                          QStringLiteral("org.deepin.dde.Appearance1"));
    readAppearanceService(QStringLiteral("com.deepin.daemon.Appearance"),
                          QStringLiteral("/com/deepin/daemon/Appearance"),
                          QStringLiteral("com.deepin.daemon.Appearance"));
}

qreal WindowStyleManager::osRadius() const
{
    return m_osRadius;
}

qreal WindowStyleManager::osScale() const
{
    return m_scale;
}

QString WindowStyleManager::activeColor() const
{
    return m_activeColor;
}

qreal WindowStyleManager::fontSizeScale() const
{
    return m_fontSizeScale;
}

QString WindowStyleManager::fontFamily() const
{
    return m_fontFamily;
}

bool WindowStyleManager::isDarkTheme() const
{
    return m_darkTheme;
}

bool WindowStyleManager::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance() && event->type() == QEvent::ApplicationPaletteChange) {
        setDarkTheme(QGuiApplication::palette().color(QPalette::Window).lightness() < 128);
        if (!m_hasAppearanceAccent) {
            m_activeColor = QGuiApplication::palette().color(QPalette::Highlight).name();
        }
    }
    return QObject::eventFilter(watched, event);
}

void WindowStyleManager::readAppearanceService(const QString &service, const QString &path, const QString &interfaceName)
{
    QDBusConnection::sessionBus().connect(service,
                                          path,
                                          QString::fromLatin1(s_appearanceInterface),
                                          QStringLiteral("PropertiesChanged"),
                                          this,
                                          SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    QDBusInterface appearance(service, path, interfaceName, QDBusConnection::sessionBus());
    if (!appearance.isValid()) {
        return;
    }
    const QVariant radius = appearance.property("WindowRadius");
    if (radius.isValid()) {
        setOsRadius(radius.toDouble());
    }
    const QVariant theme = appearance.property("GtkTheme");
    if (theme.isValid()) {
        setDarkTheme(theme.toString().contains(QStringLiteral("dark"), Qt::CaseInsensitive));
    }
    setActiveColor(appearance.property("QtActiveColor").toString());
    setIconTheme(appearance.property("IconTheme").toString());
    setFontSize(appearance.property("FontSize").toDouble());
    setFontFamily(appearance.property("StandardFont").toString());
}

void WindowStyleManager::onPropertiesChanged(const QString &interfaceName,
                                             const QVariantMap &changedProperties,
                                             const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    if (invalidatedProperties.contains(QStringLiteral("QtActiveColor"))) {
        m_hasAppearanceAccent = false;
        m_activeColor = QGuiApplication::palette().color(QPalette::Highlight).name();
    }

    if (changedProperties.contains(QStringLiteral("WindowRadius"))) {
        setOsRadius(changedProperties.value(QStringLiteral("WindowRadius")).toDouble());
    }
    if (changedProperties.contains(QStringLiteral("GtkTheme"))) {
        setDarkTheme(changedProperties.value(QStringLiteral("GtkTheme")).toString().contains(QStringLiteral("dark"), Qt::CaseInsensitive));
    }
    if (changedProperties.contains(QStringLiteral("QtActiveColor"))) {
        setActiveColor(changedProperties.value(QStringLiteral("QtActiveColor")).toString());
    }
    if (changedProperties.contains(QStringLiteral("IconTheme"))) {
        setIconTheme(changedProperties.value(QStringLiteral("IconTheme")).toString());
    }
    if (changedProperties.contains(QStringLiteral("FontSize"))) {
        setFontSize(changedProperties.value(QStringLiteral("FontSize")).toDouble());
    }
    if (changedProperties.contains(QStringLiteral("StandardFont"))) {
        setFontFamily(changedProperties.value(QStringLiteral("StandardFont")).toString());
    }
}

void WindowStyleManager::setActiveColor(const QString &color)
{
    const QColor accent(color);
    if (!accent.isValid()) {
        return;
    }
    m_hasAppearanceAccent = true;
    m_activeColor = accent.name();
}

void WindowStyleManager::setIconTheme(const QString &theme)
{
    if (theme.isEmpty() || theme == QIcon::themeName()) {
        return;
    }
    QIcon::setThemeName(theme);
    Q_EMIT iconThemeChanged();
}

void WindowStyleManager::setFontSize(qreal size)
{
    if (size > 0 && size < 100) {
        m_fontSizeScale = std::clamp(size / 10.5, qreal(0.5), qreal(3));
    }
}

void WindowStyleManager::setFontFamily(const QString &family)
{
    if (!family.isEmpty()) {
        m_fontFamily = family;
    }
}

void WindowStyleManager::setOsRadius(qreal radius)
{
    radius = std::clamp(radius, qreal(0), qreal(100));
    if (qFuzzyCompare(m_osRadius, radius)) {
        return;
    }
    m_osRadius = radius;
    Q_EMIT radiusChanged(radius);
    refreshAllWindows();
}

void WindowStyleManager::setDarkTheme(bool dark)
{
    if (m_darkTheme == dark) {
        return;
    }
    m_darkTheme = dark;
    Q_EMIT themeChanged(dark);
    refreshAllWindows();
}

void WindowStyleManager::onWindowAdded(Window *window)
{
    if (!window) {
        return;
    }
    window->createWinStyle();
    refreshWindow(window);

    connect(window, &Window::maximizedChanged, this, [this, window]() {
        refreshWindow(window);
    });
    connect(window, &Window::fullScreenChanged, this, [this, window]() {
        refreshWindow(window);
    });
    connect(window, &Window::decorationChanged, this, [this, window]() {
        refreshWindow(window);
    });
    connect(window, &Window::activeChanged, this, [window]() {
        window->updateWindowShadow();
    });
}

void WindowStyleManager::refreshWindow(Window *window)
{
    if (!window) {
        return;
    }
    window->createWinStyle();
    window->windowStyleObj()->syncX11Properties();
    window->updateWinProperty();
    window->updateWindowRadius();
    window->updateWindowShadow();
}

void WindowStyleManager::refreshAllWindows()
{
    for (Window *window : workspace()->windows()) {
        refreshWindow(window);
    }
}

} // namespace KWin

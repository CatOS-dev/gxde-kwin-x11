/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "windowstylemanager.h"

#include "window.h"
#include "windowstyle/decorationstyle.h"
#include "workspace.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QEvent>
#include <QGuiApplication>
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

bool WindowStyleManager::isDarkTheme() const
{
    return m_darkTheme;
}

bool WindowStyleManager::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance() && event->type() == QEvent::ApplicationPaletteChange) {
        setDarkTheme(QGuiApplication::palette().color(QPalette::Window).lightness() < 128);
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
}

void WindowStyleManager::onPropertiesChanged(const QString &interfaceName,
                                             const QVariantMap &changedProperties,
                                             const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    if (changedProperties.contains(QStringLiteral("WindowRadius"))) {
        setOsRadius(changedProperties.value(QStringLiteral("WindowRadius")).toDouble());
    }
    if (changedProperties.contains(QStringLiteral("GtkTheme"))) {
        setDarkTheme(changedProperties.value(QStringLiteral("GtkTheme")).toString().contains(QStringLiteral("dark"), Qt::CaseInsensitive));
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

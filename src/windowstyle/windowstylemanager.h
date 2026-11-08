/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace KWin
{

class Window;

class WindowStyleManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowStyleManager(QObject *parent = nullptr);

    qreal osRadius() const;
    qreal osScale() const;
    QString activeColor() const;
    qreal fontSizeScale() const;
    QString fontFamily() const;
    bool isDarkTheme() const;
    void setDarkTheme(bool dark);

    void refreshWindow(Window *window);

Q_SIGNALS:
    void radiusChanged(qreal radius);
    void themeChanged(bool dark);
    void scaleChanged(qreal scale);
    void iconThemeChanged();

private Q_SLOTS:
    void onWindowAdded(KWin::Window *window);
    void onPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void readAppearanceService(const QString &service, const QString &path, const QString &interfaceName);
    void setOsRadius(qreal radius);
    void setActiveColor(const QString &color);
    void setIconTheme(const QString &theme);
    void setFontSize(qreal size);
    void setFontFamily(const QString &family);
    void refreshAllWindows();

    qreal m_osRadius = 18;
    qreal m_scale = 1;
    bool m_darkTheme = false;
    QString m_activeColor;
    QString m_fontFamily;
    qreal m_fontSizeScale = 1;
    bool m_hasAppearanceAccent = false;
};

} // namespace KWin

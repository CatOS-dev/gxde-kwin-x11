/*
    SPDX-FileCopyrightText: 2024 zhang yu <zhangyud@uniontech.com>
    SPDX-FileCopyrightText: 2026 GXDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QColor>
#include <QMarginsF>
#include <QObject>
#include <QPointF>

namespace KWin
{

class Window;

class DecorationStyle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 validProperties READ validProperties WRITE setValidProperties NOTIFY validPropertiesChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QPointF windowRadius READ windowRadius WRITE setWindowRadius NOTIFY windowRadiusChanged)
    Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)
    Q_PROPERTY(qreal shadowRadius READ shadowRadius WRITE setShadowRadius NOTIFY shadowRadiusChanged)
    Q_PROPERTY(QPointF shadowOffset READ shadowOffset WRITE setShadowOffset NOTIFY shadowOffsetChanged)
    Q_PROPERTY(QColor shadowColor READ shadowColor WRITE setShadowColor NOTIFY shadowColorChanged)
    Q_PROPERTY(QMarginsF mouseInputAreaMargins READ mouseInputAreaMargins WRITE setMouseInputAreaMargins NOTIFY mouseInputAreaMarginsChanged)
    Q_PROPERTY(qreal windowPixelRatio READ windowPixelRatio WRITE setWindowPixelRatio NOTIFY windowPixelRatioChanged)
    Q_PROPERTY(qint64 windowEffect READ windowEffect WRITE setWindowEffect NOTIFY windowEffectChanged)
    Q_PROPERTY(qreal windowStartUpEffect READ windowStartUpEffect WRITE setWindowStartUpEffect NOTIFY windowStartUpEffectChanged)

public:
    enum PropertyFlag : int {
        ThemeProperty = 0x02,
        WindowRadiusProperty = 0x04,
        BorderWidthProperty = 0x08,
        BorderColorProperty = 0x10,
        ShadowRadiusProperty = 0x20,
        ShadowOffsetProperty = 0x40,
        ShadowColorProperty = 0x80,
        MouseInputAreaMarginsProperty = 0x100,
        WindowPixelRatioProperty = 0x200,
        WindowEffectProperty = 0x400,
        WindowStartUpEffectProperty = 0x800,
    };
    Q_DECLARE_FLAGS(PropertyFlags, PropertyFlag)
    Q_FLAG(PropertyFlags)

    enum EffectScene : quint32 {
        EffectNoRadius = 0x01,
        EffectNoShadow = 0x02,
        EffectNoBorder = 0x04,
        EffectNoStart = 0x10,
        EffectNoClose = 0x20,
        EffectNoMaximize = 0x40,
        EffectNoMinimize = 0x80,
    };
    Q_DECLARE_FLAGS(EffectScenes, EffectScene)
    Q_FLAG(EffectScenes)

    explicit DecorationStyle(Window *window);

    PropertyFlags validProperties() const;
    bool propertyIsValid(PropertyFlag property) const;

    QString theme() const;
    QPointF windowRadius() const;
    qreal borderWidth() const;
    QColor borderColor() const;
    qreal shadowRadius() const;
    QPointF shadowOffset() const;
    QColor shadowColor() const;
    QMarginsF mouseInputAreaMargins() const;
    qreal windowPixelRatio() const;
    qint64 windowEffect() const;
    qreal windowStartUpEffect() const;
    EffectScenes effectScenes() const;

    bool isCancelShadow() const;
    bool isCancelRadius() const;
    void cancelShadowByUser(bool cancel);
    void cancelRadiusByUser(bool cancel);

    void syncX11Properties();

public Q_SLOTS:
    void setValidProperties(qint64 properties);
    void setTheme(const QString &theme);
    void setWindowRadius(const QPointF &radius);
    void setBorderWidth(qreal width);
    void setBorderColor(const QColor &color);
    void setShadowRadius(qreal radius);
    void setShadowOffset(const QPointF &offset);
    void setShadowColor(const QColor &color);
    void setMouseInputAreaMargins(const QMarginsF &margins);
    void setWindowPixelRatio(qreal ratio);
    void setWindowEffect(qint64 effect);
    void setWindowStartUpEffect(qreal effect);

Q_SIGNALS:
    void validPropertiesChanged(qint64 properties);
    void themeChanged();
    void windowRadiusChanged();
    void borderWidthChanged();
    void borderColorChanged();
    void shadowRadiusChanged();
    void shadowOffsetChanged();
    void shadowColorChanged();
    void mouseInputAreaMarginsChanged();
    void windowPixelRatioChanged();
    void windowEffectChanged();
    void windowStartUpEffectChanged();

private:
    QPointF variantToPoint(const QVariant &value, const QPointF &fallback = QPointF()) const;
    QMarginsF variantToMargins(const QVariant &value, const QMarginsF &fallback = QMarginsF()) const;

    Window *m_window;
    PropertyFlags m_validProperties;
    QString m_theme;
    QPointF m_windowRadius;
    qreal m_borderWidth = 0;
    QColor m_borderColor;
    qreal m_shadowRadius = 0;
    QPointF m_shadowOffset;
    QColor m_shadowColor;
    QMarginsF m_mouseInputAreaMargins;
    qreal m_windowPixelRatio = 1;
    qint64 m_windowEffect = 0;
    qreal m_windowStartUpEffect = 0;
    bool m_cancelShadow = false;
    bool m_cancelRadius = false;
    bool m_x11RadiusProperty = false;
    bool m_x11ShadowProperty = false;
};

} // namespace KWin

Q_DECLARE_OPERATORS_FOR_FLAGS(KWin::DecorationStyle::PropertyFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(KWin::DecorationStyle::EffectScenes)
Q_DECLARE_METATYPE(QMarginsF)

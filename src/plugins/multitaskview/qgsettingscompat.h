// SPDX-FileCopyrightText: 2026 GXDE Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QObject>
#include <QVariant>

struct _GSettings;
struct _GSettingsSchema;

class QGSettings : public QObject
{
    Q_OBJECT

public:
    explicit QGSettings(const QByteArray &schemaId, QObject *parent = nullptr);
    ~QGSettings() override;

    QVariant get(const QString &key) const;
    void set(const QString &key, const QVariant &value);
    bool isValid() const;

Q_SIGNALS:
    void changed(const QString &key);

private:
    QString resolveKey(const QString &key) const;

    _GSettingsSchema *m_schema = nullptr;
    _GSettings *m_settings = nullptr;
    unsigned long m_changedHandler = 0;
};

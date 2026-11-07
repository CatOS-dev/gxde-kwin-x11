// SPDX-FileCopyrightText: 2026 GXDE Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qgsettingscompat.h"

#include <gio/gio.h>

#include <vector>

namespace
{

QString kebabCase(const QString &key)
{
    QString result;
    result.reserve(key.size() + 4);
    for (const QChar character : key) {
        if (character.isUpper()) {
            result += QLatin1Char('-');
            result += character.toLower();
        } else {
            result += character;
        }
    }
    return result;
}

QVariant fromGVariant(GVariant *value)
{
    if (!value) {
        return {};
    }

    QVariant result;
    switch (g_variant_classify(value)) {
    case G_VARIANT_CLASS_BOOLEAN:
        result = QVariant::fromValue(bool(g_variant_get_boolean(value)));
        break;
    case G_VARIANT_CLASS_INT32:
        result = g_variant_get_int32(value);
        break;
    case G_VARIANT_CLASS_UINT32:
        result = g_variant_get_uint32(value);
        break;
    case G_VARIANT_CLASS_INT64:
        result = qlonglong(g_variant_get_int64(value));
        break;
    case G_VARIANT_CLASS_UINT64:
        result = qulonglong(g_variant_get_uint64(value));
        break;
    case G_VARIANT_CLASS_DOUBLE:
        result = g_variant_get_double(value);
        break;
    case G_VARIANT_CLASS_STRING:
        result = QString::fromUtf8(g_variant_get_string(value, nullptr));
        break;
    case G_VARIANT_CLASS_ARRAY:
        if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
            gsize length = 0;
            const gchar **strings = g_variant_get_strv(value, &length);
            QStringList list;
            list.reserve(length);
            for (gsize i = 0; i < length; ++i) {
                list.append(QString::fromUtf8(strings[i]));
            }
            g_free(strings);
            result = list;
        }
        break;
    default:
        break;
    }
    g_variant_unref(value);
    return result;
}

} // namespace

QGSettings::QGSettings(const QByteArray &schemaId, QObject *parent)
    : QObject(parent)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (!source) {
        return;
    }
    m_schema = g_settings_schema_source_lookup(source, schemaId.constData(), true);
    if (!m_schema) {
        return;
    }
    m_settings = g_settings_new_full(m_schema, nullptr, nullptr);
    m_changedHandler = g_signal_connect(m_settings, "changed", G_CALLBACK(+[](_GSettings *, const gchar *key, gpointer data) {
        auto self = static_cast<QGSettings *>(data);
        Q_EMIT self->changed(QString::fromUtf8(key));
    }),
                                        this);
}

QGSettings::~QGSettings()
{
    if (m_settings) {
        if (m_changedHandler) {
            g_signal_handler_disconnect(m_settings, m_changedHandler);
        }
        g_object_unref(m_settings);
    }
    if (m_schema) {
        g_settings_schema_unref(m_schema);
    }
}

bool QGSettings::isValid() const
{
    return m_settings;
}

QString QGSettings::resolveKey(const QString &key) const
{
    if (!m_schema) {
        return {};
    }
    const QByteArray direct = key.toUtf8();
    if (g_settings_schema_has_key(m_schema, direct.constData())) {
        return key;
    }
    const QString converted = kebabCase(key);
    const QByteArray convertedUtf8 = converted.toUtf8();
    if (g_settings_schema_has_key(m_schema, convertedUtf8.constData())) {
        return converted;
    }
    return {};
}

QVariant QGSettings::get(const QString &key) const
{
    const QString resolved = resolveKey(key);
    if (!m_settings || resolved.isEmpty()) {
        return {};
    }
    return fromGVariant(g_settings_get_value(m_settings, resolved.toUtf8().constData()));
}

void QGSettings::set(const QString &key, const QVariant &value)
{
    const QString resolved = resolveKey(key);
    if (!m_settings || resolved.isEmpty()) {
        return;
    }

    GVariant *variant = nullptr;
    if (value.metaType().id() == QMetaType::QStringList) {
        const QStringList list = value.toStringList();
        QList<QByteArray> utf8;
        utf8.reserve(list.size());
        std::vector<const gchar *> strings;
        strings.reserve(list.size());
        for (const QString &entry : list) {
            utf8.append(entry.toUtf8());
            strings.push_back(utf8.constLast().constData());
        }
        variant = g_variant_new_strv(strings.data(), strings.size());
    } else if (value.metaType().id() == QMetaType::QString) {
        variant = g_variant_new_string(value.toString().toUtf8().constData());
    } else if (value.metaType().id() == QMetaType::Bool) {
        variant = g_variant_new_boolean(value.toBool());
    } else {
        variant = g_variant_new_int32(value.toInt());
    }
    g_settings_set_value(m_settings, resolved.toUtf8().constData(), variant);
}

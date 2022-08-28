/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "cursor_theme.h"

#include "cursor.h"

#include "base/platform.h"
#include "base/wayland/server.h"
#include "main.h"
#include "win/space.h"

#include <QVector>

#include <Wrapland/Client/shm_pool.h>
#include <Wrapland/Server/display.h>
#include <Wrapland/Server/output.h>

#include <wayland-cursor.h>

namespace KWin::input::wayland
{

cursor_theme::cursor_theme(wayland::cursor& cursor, Wrapland::Client::ShmPool* shm)
    : qobject{std::make_unique<cursor_theme_qobject>()}
    , cursor{cursor}
    , m_shm{shm}
{
    QObject::connect(&kwinApp()->get_base(),
                     &base::platform::topology_changed,
                     qobject.get(),
                     [this](auto old, auto topo) {
                         if (old.max_scale != topo.max_scale) {
                             loadTheme();
                         }
                     });
}

cursor_theme::~cursor_theme()
{
    destroyTheme();
}

void cursor_theme::loadTheme()
{
    if (!m_shm->isValid()) {
        return;
    }

    int size = cursor.theme_size();
    if (size == 0) {
        // set a default size
        size = 24;
    }

    size *= kwinApp()->get_base().topology.max_scale;

    auto theme = wl_cursor_theme_load(cursor.theme_name().toUtf8().constData(), size, m_shm->shm());
    if (theme) {
        if (!m_theme) {
            // so far the theme had not been created, this means we need to start tracking theme
            // changes
            QObject::connect(
                &cursor, &input::cursor::theme_changed, qobject.get(), [this] { loadTheme(); });
        } else {
            destroyTheme();
        }
        m_theme = theme;
        Q_EMIT qobject->themeChanged();
    }
}

void cursor_theme::destroyTheme()
{
    if (!m_theme) {
        return;
    }
    wl_cursor_theme_destroy(m_theme);
    m_theme = nullptr;
}

wl_cursor_image* cursor_theme::get(input::cursor_shape shape)
{
    return get(shape.name());
}

wl_cursor_image* cursor_theme::get(const QByteArray& name)
{
    if (!m_theme) {
        loadTheme();
    }
    if (!m_theme) {
        // loading cursor failed
        return nullptr;
    }
    wl_cursor* c = wl_cursor_theme_get_cursor(m_theme, name.constData());
    if (!c || c->image_count <= 0) {
        const auto& names = cursor.alternative_names(name);
        for (auto it = names.begin(), end = names.end(); it != end; it++) {
            c = wl_cursor_theme_get_cursor(m_theme, (*it).constData());
            if (c && c->image_count > 0) {
                break;
            }
        }
    }
    if (!c || c->image_count <= 0) {
        return nullptr;
    }
    // TODO: who deletes c?
    return c->images[0];
}

}

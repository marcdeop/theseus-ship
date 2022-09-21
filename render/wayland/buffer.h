/*
    SPDX-FileCopyrightText: 2022 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "render/buffer.h"

#include <QImage>
#include <Wrapland/Server/buffer.h>
#include <Wrapland/Server/surface.h>
#include <memory>

class QOpenGLFramebufferObject;

namespace KWin::render::wayland
{

template<typename Buffer>
struct buffer_win_integration : public render::buffer_win_integration<Buffer> {
public:
    buffer_win_integration(Buffer const& buffer)
        : render::buffer_win_integration<Buffer>(buffer)
    {
    }

    bool valid() const override
    {
        return external || internal.fbo || !internal.image.isNull();
    }

    QRegion damage() const override
    {
        if (external) {
            if (auto surf = this->buffer.window->ref_win->surface) {
                return surf->trackedDamage();
            }
            return {};
        }
        if (internal.fbo || !internal.image.isNull()) {
            return this->buffer.window->ref_win->render_data.damage_region;
        }
        return {};
    }

    std::shared_ptr<Wrapland::Server::Buffer> external;
    struct {
        std::shared_ptr<QOpenGLFramebufferObject> fbo;
        QImage image;
    } internal;
};

}

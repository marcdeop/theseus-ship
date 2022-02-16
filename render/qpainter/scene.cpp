/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "scene.h"

#include "deco_renderer.h"
#include "effect_frame.h"
#include "shadow.h"
#include "window.h"

#include "base/output.h"
#include "base/platform.h"
#include "decorations/decoratedclient.h"
#include "input/cursor.h"
#include "main.h"
#include "render/compositor.h"
#include "render/cursor.h"
#include "render/platform.h"
#include "toplevel.h"
#include "wayland_logging.h"
#include "win/space.h"

#include <kwineffects/effects_handler.h>

#include <KDecoration2/Decoration>
#include <QPainter>
#include <Wrapland/Server/buffer.h>
#include <Wrapland/Server/surface.h>

#include <cmath>

namespace KWin::render::qpainter
{

scene::scene(qpainter::backend* backend, render::compositor& compositor)
    : render::scene(compositor)
    , m_backend(backend)
    , m_painter(new QPainter())
{
}

scene::~scene()
{
}

CompositingType scene::compositingType() const
{
    return QPainterCompositing;
}

bool scene::initFailed() const
{
    return false;
}

void scene::paintGenericScreen(paint_type mask, ScreenPaintData data)
{
    m_painter->save();
    m_painter->translate(data.xTranslation(), data.yTranslation());
    m_painter->scale(data.xScale(), data.yScale());
    render::scene::paintGenericScreen(mask, data);
    m_painter->restore();
}

int64_t scene::paint_output(base::output* output,
                            QRegion damage,
                            std::deque<Toplevel*> const& toplevels,
                            std::chrono::milliseconds presentTime)
{
    QElapsedTimer renderTimer;
    renderTimer.start();

    createStackingOrder(toplevels);

    auto mask = paint_type::none;
    m_backend->begin_render(*output);

    auto const needsFullRepaint = m_backend->needsFullRepaint();
    if (needsFullRepaint) {
        mask |= render::paint_type::screen_background_first;
        damage = QRect({}, kwinApp()->get_base().topology.size);
    }

    auto const geometry = output->geometry();

    auto buffer = m_backend->bufferForScreen(output);
    if (!buffer || buffer->isNull()) {
        return renderTimer.nsecsElapsed();
    }

    m_painter->begin(buffer);
    m_painter->save();
    m_painter->setWindow(geometry);

    repaint_output = output;
    QRegion updateRegion, validRegion;

    paintScreen(
        mask, damage.intersected(geometry), QRegion(), &updateRegion, &validRegion, presentTime);
    paintCursor();

    m_painter->restore();
    m_painter->end();

    m_backend->present(output, updateRegion);

    clearStackingOrder();
    Q_EMIT frameRendered();

    return renderTimer.nsecsElapsed();
}

void scene::paintBackground(QRegion region)
{
    m_painter->setBrush(Qt::black);
    for (const QRect& rect : region) {
        m_painter->drawRect(rect);
    }
}

void scene::paintCursor()
{
    auto cursor = render::compositor::self()->software_cursor.get();
    if (!cursor->enabled) {
        return;
    }
    auto const img = cursor->image();
    if (img.isNull()) {
        return;
    }
    auto const cursorPos = input::get_cursor()->pos();
    auto const hotspot = cursor->hotspot();
    m_painter->drawImage(cursorPos - hotspot, img);
    cursor->mark_as_rendered();
}

void scene::paintEffectQuickView(EffectQuickView* w)
{
    QPainter* painter = effects->scenePainter();
    const QImage buffer = w->bufferAsImage();
    if (buffer.isNull()) {
        return;
    }
    painter->save();
    painter->setOpacity(w->opacity());
    painter->drawImage(w->geometry(), buffer);
    painter->restore();
}

std::unique_ptr<render::window> scene::createWindow(Toplevel* toplevel)
{
    return std::make_unique<window>(this, toplevel);
}

render::effect_frame* scene::createEffectFrame(effect_frame_impl* frame)
{
    return new effect_frame(frame, this);
}

render::shadow* scene::createShadow(Toplevel* toplevel)
{
    return new shadow(toplevel);
}

Decoration::Renderer* scene::createDecorationRenderer(Decoration::DecoratedClientImpl* impl)
{
    return new deco_renderer(impl);
}

void scene::handle_screen_geometry_change(QSize const& /*size*/)
{
}

backend* create_backend(render::compositor& compositor)
{
    try {
        return compositor.platform.createQPainterBackend(compositor);
    } catch (std::runtime_error& error) {
        qCWarning(KWIN_WL) << "Creating QPainter backend failed:" << error.what();
        return nullptr;
    }
}

render::scene* create_scene(render::compositor& compositor)
{
    auto backend = create_backend(compositor);
    if (!backend) {
        return nullptr;
    }

    auto scene = new qpainter::scene(backend, compositor);

    if (scene && scene->initFailed()) {
        delete scene;
        scene = nullptr;
    }
    return scene;
}

}

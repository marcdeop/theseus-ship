/*
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "forward.h"

#include "../event.h"
#include "../keyboard_redirect.h"
#include "../qt_event.h"
#include "../redirect.h"
#include "../touch_redirect.h"
#include "main.h"
#include "wayland_server.h"
#include "workspace.h"
#include <input/pointer_redirect.h>

#include <Wrapland/Server/pointer_pool.h>
#include <Wrapland/Server/seat.h>
#include <Wrapland/Server/touch_pool.h>

namespace KWin::input
{

bool forward_filter::key(key_event const& event)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    kwinApp()->input->redirect->keyboard()->update();
    seat->setTimestamp(event.base.time_msec);
    passToWaylandServer(event);
    return true;
}

bool forward_filter::button(button_event const& event)
{
    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);

    switch (event.state) {
    case button_state::pressed:
        seat->pointers().button_pressed(event.key);
        break;
    case button_state::released:
        seat->pointers().button_released(event.key);
        break;
    }

    return true;
}

bool forward_filter::motion(motion_event const& event)
{
    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);

    seat->pointers().set_position(kwinApp()->input->redirect->pointer()->pos());
    if (!event.delta.isNull()) {
        seat->pointers().relative_motion(QSizeF(event.delta.x(), event.delta.y()),
                                         QSizeF(event.unaccel_delta.x(), event.unaccel_delta.y()),
                                         event.base.time_msec);
    }

    return true;
}

bool forward_filter::touchDown(qint32 id, const QPointF& pos, quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    kwinApp()->input->redirect->touch()->insertId(id, seat->touches().touch_down(pos));
    return true;
}
bool forward_filter::touchMotion(qint32 id, const QPointF& pos, quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    const qint32 wraplandId = kwinApp()->input->redirect->touch()->mappedId(id);
    if (wraplandId != -1) {
        seat->touches().touch_move(wraplandId, pos);
    }
    return true;
}
bool forward_filter::touchUp(qint32 id, quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    const qint32 wraplandId = kwinApp()->input->redirect->touch()->mappedId(id);
    if (wraplandId != -1) {
        seat->touches().touch_up(wraplandId);
        kwinApp()->input->redirect->touch()->removeId(id);
    }
    return true;
}

bool forward_filter::axis(axis_event const& event)
{
    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);

    using wrap_source = Wrapland::Server::PointerAxisSource;

    auto source = wrap_source::Unknown;
    switch (event.source) {
    case axis_source::wheel:
        source = wrap_source::Wheel;
        break;
    case axis_source::finger:
        source = wrap_source::Finger;
        break;
    case axis_source::continuous:
        source = wrap_source::Continuous;
        break;
    case axis_source::wheel_tilt:
        source = wrap_source::WheelTilt;
        break;
    case axis_source::unknown:
    default:
        source = wrap_source::Unknown;
        break;
    }

    auto orientation = (event.orientation == axis_orientation::horizontal)
        ? Qt::Orientation::Horizontal
        : Qt::Orientation::Vertical;

    seat->pointers().send_axis(orientation, event.delta, event.delta_discrete, source);
    return true;
}

bool forward_filter::pinch_begin(pinch_begin_event const& event)
{
    if (!workspace()) {
        return false;
    }

    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);
    seat->pointers().start_pinch_gesture(event.fingers);

    return true;
}

bool forward_filter::pinch_update(pinch_update_event const& event)
{
    if (!workspace()) {
        return false;
    }

    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);
    seat->pointers().update_pinch_gesture(
        QSize(event.delta.x(), event.delta.y()), event.scale, event.rotation);

    return true;
}

bool forward_filter::pinch_end(pinch_end_event const& event)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(event.base.time_msec);

    if (event.cancelled) {
        seat->pointers().cancel_pinch_gesture();
    } else {
        seat->pointers().end_pinch_gesture();
    }

    return true;
}

bool forward_filter::swipeGestureBegin(int fingerCount, quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    seat->pointers().start_swipe_gesture(fingerCount);
    return true;
}

bool forward_filter::swipeGestureUpdate(const QSizeF& delta, quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    seat->pointers().update_swipe_gesture(delta);
    return true;
}

bool forward_filter::swipeGestureEnd(quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    seat->pointers().end_swipe_gesture();
    return true;
}

bool forward_filter::swipeGestureCancelled(quint32 time)
{
    if (!workspace()) {
        return false;
    }
    auto seat = waylandServer()->seat();
    seat->setTimestamp(time);
    seat->pointers().cancel_swipe_gesture();
    return true;
}

}

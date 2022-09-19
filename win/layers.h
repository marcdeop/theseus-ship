/*
    SPDX-FileCopyrightText: ...

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "net.h"
#include "transient.h"
#include "types.h"

#include "utils/blocker.h"

namespace KWin::win
{

/**
 * Window that was activated, but it's not yet really active_client, because
 * we didn't process yet the matching FocusIn event. Used mostly in focus
 * stealing prevention code.
 */
template<typename Space>
typename Space::window_t* most_recently_activated_window(Space const& space)
{
    auto const& candidates = space.stacking.should_get_focus;
    return candidates.size() > 0 ? candidates.back() : space.stacking.active;
}

template<typename Win>
bool is_active_fullscreen(Win const* win)
{
    if (!win->control->fullscreen) {
        return false;
    }

    // Instead of activeClient() - avoids flicker.
    auto const ac = most_recently_activated_window(win->space);

    // According to NETWM spec implementation notes suggests "focused windows having state
    // _NET_WM_STATE_FULLSCREEN" to be on the highest layer. Also take the screen into account.
    return ac
        && (ac == win || ac->central_output != win->central_output
            || contains(ac->transient->leads(), win));
}

template<typename Win>
layer layer_for_dock(Win const& win)
{
    assert(win.control);

    // Slight hack for the 'allow window to cover panel' Kicker setting.
    // Don't move keepbelow docks below normal window, but only to the same
    // layer, so that both may be raised to cover the other.
    if (win.control->keep_below) {
        return win::layer::normal;
    }
    if (win.control->keep_above) {
        // slight hack for the autohiding panels
        return win::layer::above;
    }
    return win::layer::dock;
}

template<typename Win>
layer belong_to_layer(Win* win)
{
    // NOTICE while showingDesktop, desktops move to the AboveLayer
    // (interchangeable w/ eg. yakuake etc. which will at first remain visible)
    // and the docks move into the NotificationLayer (which is between Above- and
    // ActiveLayer, so that active fullscreen windows will still cover everything)
    // Since the desktop is also activated, nothing should be in the ActiveLayer, though
    if (win->isInternal()) {
        return win::layer::unmanaged;
    }
    if (win->isLockScreen()) {
        return win::layer::unmanaged;
    }
    if (is_desktop(win)) {
        return win->space.showing_desktop ? win::layer::above : win::layer::desktop;
    }
    if (is_splash(win)) {
        return win::layer::normal;
    }
    if (is_popup(win)) {
        return win::layer::popup;
    }
    if (is_dock(win) || is_applet_popup(win)) {
        if (win->space.showing_desktop) {
            return win::layer::notification;
        }
        return win->layer_for_dock();
    }
    if (is_on_screen_display(win)) {
        return win::layer::on_screen_display;
    }
    if (is_notification(win)) {
        return win::layer::notification;
    }
    if (is_critical_notification(win)) {
        return win::layer::critical_notification;
    }
    if (win->space.showing_desktop && win->belongsToDesktop()) {
        return win::layer::above;
    }
    if (win->control->keep_below) {
        return win::layer::below;
    }
    if (is_active_fullscreen(win)) {
        return win::layer::active;
    }
    if (win->control->keep_above) {
        return win::layer::above;
    }
    return win::layer::normal;
}

// TODO(romangg): Setting the cache for the layer lazily here is a bit unusual. Maybe instead make
//                this a simple getter and call belong_to_layer explicitly when appropriate.
template<typename Win>
layer get_layer(Win const& win)
{
    if (win.transient->lead() && win.transient->annexed) {
        return get_layer(*win.transient->lead());
    }
    if (win.layer == layer::unknown) {
        const_cast<Win&>(win).layer = belong_to_layer(&win);
    }
    return win.layer;
}

template<typename Win>
void invalidate_layer(Win* win)
{
    win->layer = layer::unknown;
}

template<typename Win>
void update_layer(Win* win)
{
    if (!win) {
        return;
    }
    if (win->remnant || get_layer(*win) == belong_to_layer(win)) {
        return;
    }

    blocker block(win->space.stacking.order);

    // Invalidate, will be updated when doing restacking.
    invalidate_layer(win);

    for (auto const& child : win->transient->children) {
        if (!child->transient->annexed) {
            update_layer(child);
        }
    }
}

}

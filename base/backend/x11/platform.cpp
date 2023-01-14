/*
    SPDX-FileCopyrightText: 2022 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "platform.h"

#include "randr_filter.h"

#include "base/x11/xcb/randr.h"
#include "output_helpers.h"

#include "base/output_helpers.h"
#include "x11_logging.h"

namespace KWin::base::backend::x11
{

platform::platform(base::config config)
    : base::x11::platform(std::move(config))
{
}

void platform::update_outputs()
{
    if (!randr_filter) {
        randr_filter = std::make_unique<RandrFilter>(this);
        update_outputs_impl<base::x11::xcb::randr::screen_resources>();
        return;
    }

    update_outputs_impl<base::x11::xcb::randr::current_resources>();
}

template<typename Resources>
void platform::update_outputs_impl()
{
    auto res_outs = get_outputs_from_resources(*this, Resources(rootWindow()));

    qCDebug(KWIN_X11) << "Update outputs:" << this->outputs.size() << "-->" << res_outs.size();

    // First check for removed outputs (we go backwards through the outputs, LIFO).
    for (auto old_it = this->outputs.rbegin(); old_it != this->outputs.rend();) {
        auto x11_old_out = static_cast<base::x11::output*>(*old_it);

        auto is_in_new_outputs = [x11_old_out, &res_outs] {
            auto it
                = std::find_if(res_outs.begin(), res_outs.end(), [x11_old_out](auto const& out) {
                      return x11_old_out->data.crtc == out->data.crtc
                          && x11_old_out->data.name == out->data.name;
                  });
            return it != res_outs.end();
        };

        if (is_in_new_outputs()) {
            // The old output is still there. Keep it in the base outputs.
            old_it++;
            continue;
        }

        qCDebug(KWIN_X11) << "  removed:" << x11_old_out->name();
        auto old_out = *old_it;
        old_it = static_cast<decltype(old_it)>(this->outputs.erase(std::next(old_it).base()));
        Q_EMIT output_removed(old_out);
        delete old_out;
    }

    // Second check for added outputs.
    for (auto& out : res_outs) {
        auto it
            = std::find_if(this->outputs.begin(), this->outputs.end(), [&out](auto const& old_out) {
                  auto old_x11_out = static_cast<base::x11::output*>(old_out);
                  return old_x11_out->data.crtc == out->data.crtc
                      && old_x11_out->data.name == out->data.name;
              });
        if (it == this->outputs.end()) {
            qCDebug(KWIN_X11) << "  added:" << out->name();
            this->outputs.push_back(out.release());
            Q_EMIT output_added(this->outputs.back());
        }
    }

    update_output_topology(*this);
}

}

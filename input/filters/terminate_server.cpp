/*
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "terminate_server.h"

#include "input/event.h"
#include "input/keyboard_redirect.h"
#include "input/logging.h"
#include "input/xkb.h"
#include "main.h"

#include "utils.h"

namespace KWin::input
{

bool terminate_server_filter::key(key_event const& event)
{
    if (event.state == key_state::pressed) {
        auto const& xkb = kwinApp()->input->redirect->keyboard()->xkb();
        if (xkb->toKeysym(event.keycode) == XKB_KEY_Terminate_Server) {
            qCWarning(KWIN_INPUT) << "Request to terminate server";
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}

}

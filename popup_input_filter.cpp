/*
 * Copyright 2017  Martin Graesslin <mgraesslin@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "popup_input_filter.h"
#include "wayland_server.h"

#include "win/deco.h"
#include "win/geo.h"
#include "win/util.h"
#include "win/wayland/window.h"

#include <QMouseEvent>

namespace KWin
{

PopupInputFilter::PopupInputFilter()
    : QObject()
{
    connect(waylandServer(), &WaylandServer::window_added, this, &PopupInputFilter::handleClientAdded);
}

void PopupInputFilter::handleClientAdded(Toplevel *client)
{
    if (contains(m_popups, client)) {
        return;
    }
    if (client->hasPopupGrab()) {
        // TODO: verify that the Toplevel is allowed as a popup
        connect(client, &Toplevel::windowShown, this, &PopupInputFilter::handleClientAdded, Qt::UniqueConnection);
        connect(client, &Toplevel::windowClosed, this, &PopupInputFilter::handleClientRemoved, Qt::UniqueConnection);
        m_popups.push_back(client);
    }
}

void PopupInputFilter::handleClientRemoved(Toplevel *client)
{
    remove_all(m_popups, client);
}
bool PopupInputFilter::pointerEvent(QMouseEvent *event, quint32 nativeButton)
{
    Q_UNUSED(nativeButton)
    if (m_popups.empty()) {
        return false;
    }
    if (event->type() == QMouseEvent::MouseButtonPress) {
        auto pointerFocus = input()->findToplevel(event->globalPos());
        if (!pointerFocus || !win::belong_to_same_client(pointerFocus, m_popups.back())) {
            // a press on a window (or no window) not belonging to the popup window
            cancelPopups();
            // filter out this press
            return true;
        }
        if (pointerFocus && win::decoration(pointerFocus)) {
            // test whether it is on the decoration
            auto const clientRect
                = QRect(win::to_client_pos(pointerFocus, QPoint()), pointerFocus->clientSize())
                      .translated(pointerFocus->pos());
            if (!clientRect.contains(event->globalPos())) {
                cancelPopups();
                return true;
            }
        }
    }
    return false;
}

void PopupInputFilter::cancelPopups()
{
    while (!m_popups.empty()) {
        m_popups.back()->popupDone();
        m_popups.pop_back();
    }
}

}

/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2015 Martin Gräßlin <mgraesslin@kde.org>
Copyright (C) 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

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
#include "abstract_client.h"

#include "cursor.h"
#include "focuschain.h"
#include "outline.h"
#include "screens.h"
#include "screenedge.h"
#include "useractions.h"
#include "win/control.h"
#include "win/setup.h"
#include "win/win.h"
#include "workspace.h"

#include "wayland_server.h"

namespace KWin
{

AbstractClient::AbstractClient()
    : Toplevel()
{
    win::setup_connections(this);
}

AbstractClient::~AbstractClient()
{
}

bool AbstractClient::isTransient() const
{
    return false;
}

void AbstractClient::setClientShown(bool shown)
{
    Q_UNUSED(shown)
}

win::maximize_mode AbstractClient::requestedMaximizeMode() const
{
    return maximizeMode();
}

xcb_timestamp_t AbstractClient::userTime() const
{
    return XCB_TIME_CURRENT_TIME;
}

void AbstractClient::doSetActive()
{
}

void AbstractClient::doSetKeepAbove()
{
}

void AbstractClient::doSetKeepBelow()
{
}

void AbstractClient::doSetDesktop(int desktop, int was_desk)
{
    Q_UNUSED(desktop)
    Q_UNUSED(was_desk)
}

bool AbstractClient::isShadeable() const
{
    return false;
}

void AbstractClient::setShade(win::shade mode)
{
    Q_UNUSED(mode)
}

win::shade AbstractClient::shadeMode() const
{
    return win::shade::none;
}

win::position AbstractClient::titlebarPosition() const
{
    return win::position::top;
}

void AbstractClient::doMinimize()
{
}

QSize AbstractClient::maxSize() const
{
    return control()->rules().checkMaxSize(QSize(INT_MAX, INT_MAX));
}

QSize AbstractClient::minSize() const
{
    return control()->rules().checkMinSize(QSize(0, 0));
}

void AbstractClient::move(int x, int y, win::force_geometry force)
{
    // resuming geometry updates is handled only in setGeometry()
    Q_ASSERT(control()->pending_geometry_update() == win::pending_geometry::none
             || control()->geometry_updates_blocked());
    QPoint p(x, y);
    if (!control()->geometry_updates_blocked() && p != control()->rules().checkPosition(p)) {
        qCDebug(KWIN_CORE) << "forced position fail:" << p << ":" << control()->rules().checkPosition(p);
    }
    if (force == win::force_geometry::no && m_frameGeometry.topLeft() == p)
        return;
    auto old_frame_geometry = m_frameGeometry;
    m_frameGeometry.moveTopLeft(p);
    if (control()->geometry_updates_blocked()) {
        if (control()->pending_geometry_update() == win::pending_geometry::forced)
            {} // maximum, nothing needed
        else if (force == win::force_geometry::yes)
            control()->set_pending_geometry_update(win::pending_geometry::forced);
        else
            control()->set_pending_geometry_update(win::pending_geometry::normal);
        return;
    }
    doMove(x, y);
    updateWindowRules(Rules::Position);
    screens()->setCurrent(this);
    workspace()->updateStackingOrder();
    // client itself is not damaged
    win::add_repaint_during_geometry_updates(this);
    control()->update_geometry_before_update_blocking();
    emit geometryChanged();
    Q_EMIT frameGeometryChanged(this, old_frame_geometry);
}

bool AbstractClient::hasStrut() const
{
    return false;
}

bool AbstractClient::performMouseCommand(Options::MouseCommand cmd, const QPoint &globalPos)
{
    return win::perform_mouse_command(this, cmd, globalPos);
}

bool AbstractClient::hasTransientPlacementHint() const
{
    return false;
}

QRect AbstractClient::transientPlacement(const QRect &bounds) const
{
    Q_UNUSED(bounds);
    Q_UNREACHABLE();
    return QRect();
}

QList< AbstractClient* > AbstractClient::mainClients() const
{
    if (auto t = dynamic_cast<const AbstractClient *>(control()->transient_lead())) {
        return QList<AbstractClient*>{const_cast< AbstractClient* >(t)};
    }
    return QList<AbstractClient*>();
}

QSize AbstractClient::sizeForClientSize(const QSize &wsize,
                                        [[maybe_unused]] win::size_mode mode,
                                        [[maybe_unused]] bool noframe) const
{
    return wsize + QSize(win::left_border(this) + win::right_border(this),
                         win::top_border(this) + win::bottom_border(this));
}

void AbstractClient::doMove(int, int)
{
}

void AbstractClient::leaveMoveResize()
{
    workspace()->setMoveResizeClient(nullptr);
    control()->move_resize().enabled = false;
    if (ScreenEdges::self()->isDesktopSwitchingMovingClients())
        ScreenEdges::self()->reserveDesktopSwitching(false, Qt::Vertical|Qt::Horizontal);
    if (control()->electric_maximizing()) {
        outline()->hide();
        win::elevate(this, false);
    }
}

bool AbstractClient::doStartMoveResize()
{
    return true;
}

void AbstractClient::positionGeometryTip()
{
}

void AbstractClient::doPerformMoveResize()
{
}

bool AbstractClient::isWaitingForMoveResizeSync() const
{
    return false;
}

void AbstractClient::doResizeSync()
{
}

QSize AbstractClient::resizeIncrements() const
{
    return QSize(1, 1);
}

void AbstractClient::layoutDecorationRects(QRect &left, QRect &top, QRect &right, QRect &bottom) const
{
    win::layout_decoration_rects(this, left, top, right, bottom);
}

bool AbstractClient::providesContextHelp() const
{
    return false;
}

void AbstractClient::showContextHelp()
{
}

QRect AbstractClient::iconGeometry() const
{
    auto management = control()->wayland_management();
    if (!management || !waylandServer()) {
        // window management interface is only available if the surface is mapped
        return QRect();
    }

    int minDistance = INT_MAX;
    AbstractClient *candidatePanel = nullptr;
    QRect candidateGeom;

    for (auto i = management->minimizedGeometries().constBegin(),
         end = management->minimizedGeometries().constEnd(); i != end; ++i) {
        AbstractClient *client = waylandServer()->findAbstractClient(i.key());
        if (!client) {
            continue;
        }
        const int distance = QPoint(client->pos() - pos()).manhattanLength();
        if (distance < minDistance) {
            minDistance = distance;
            candidatePanel = client;
            candidateGeom = i.value();
        }
    }
    if (!candidatePanel) {
        return QRect();
    }
    return candidateGeom.translated(candidatePanel->pos());
}

QRect AbstractClient::inputGeometry() const
{
    if (auto& deco = control()->deco(); deco.enabled()) {
        return Toplevel::inputGeometry() + deco.decoration->resizeOnlyBorders();
    }
    return Toplevel::inputGeometry();
}

bool AbstractClient::dockWantsInput() const
{
    return false;
}

void AbstractClient::setOnActivities(QStringList newActivitiesList)
{
    Q_UNUSED(newActivitiesList)
}

void AbstractClient::checkNoBorder()
{
    setNoBorder(false);
}

bool AbstractClient::groupTransient() const
{
    return false;
}

const Group *AbstractClient::group() const
{
    return nullptr;
}

Group *AbstractClient::group()
{
    return nullptr;
}

bool AbstractClient::supportsWindowRules() const
{
    return true;
}

QMargins AbstractClient::frameMargins() const
{
    return QMargins(win::left_border(this), win::top_border(this),
                    win::right_border(this), win::bottom_border(this));
}

QPoint AbstractClient::framePosToClientPos(const QPoint &point) const
{
    return point + QPoint(win::left_border(this), win::top_border(this));
}

QPoint AbstractClient::clientPosToFramePos(const QPoint &point) const
{
    return point - QPoint(win::left_border(this), win::top_border(this));
}

QSize AbstractClient::frameSizeToClientSize(const QSize &size) const
{
    const int width = size.width() - win::left_border(this) - win::right_border(this);
    const int height = size.height() - win::top_border(this) - win::bottom_border(this);
    return QSize(width, height);
}

QSize AbstractClient::clientSizeToFrameSize(const QSize &size) const
{
    const int width = size.width() + win::left_border(this) + win::right_border(this);
    const int height = size.height() + win::top_border(this) + win::bottom_border(this);
    return QSize(width, height);
}

QSize AbstractClient::basicUnit() const
{
    return QSize(1, 1);
}

void AbstractClient::setBlockingCompositing([[maybe_unused]] bool block)
{
}

bool AbstractClient::isBlockingCompositing()
{
    return false;
}

QPoint AbstractClient::clientPos() const
{
    return QPoint(win::left_border(this), win::top_border(this));
}

}

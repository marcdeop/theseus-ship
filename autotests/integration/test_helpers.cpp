/*
    SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "kwin_wayland_test.h"
#include "screenlockerwatcher.h"
#include "wayland_server.h"

#include "win/wayland/window.h"

#include <Wrapland/Client/appmenu.h>
#include <Wrapland/Client/compositor.h>
#include <Wrapland/Client/connection_thread.h>
#include <Wrapland/Client/event_queue.h>
#include <Wrapland/Client/idleinhibit.h>
#include <Wrapland/Client/layer_shell_v1.h>
#include <Wrapland/Client/output.h>
#include <Wrapland/Client/plasmashell.h>
#include <Wrapland/Client/plasmawindowmanagement.h>
#include <Wrapland/Client/pointerconstraints.h>
#include <Wrapland/Client/registry.h>
#include <Wrapland/Client/seat.h>
#include <Wrapland/Client/shadow.h>
#include <Wrapland/Client/shm_pool.h>
#include <Wrapland/Client/subcompositor.h>
#include <Wrapland/Client/subsurface.h>
#include <Wrapland/Client/surface.h>
#include <Wrapland/Client/xdg_shell.h>
#include <Wrapland/Client/xdgdecoration.h>
#include <Wrapland/Server/display.h>

#include <KScreenLocker/KsldApp>

#include <QThread>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace Clt = Wrapland::Client;

namespace KWin
{
namespace Test
{

client s_waylandConnection;

client::client(AdditionalWaylandInterfaces flags)
{
    int sx[2];
    QVERIFY(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sx) >= 0);

    KWin::waylandServer()->display()->createClient(sx[0]);

    // Setup connection.
    connection = new Clt::ConnectionThread;

    QSignalSpy connectedSpy(connection, &Clt::ConnectionThread::establishedChanged);
    QVERIFY(connectedSpy.isValid());

    connection->setSocketFd(sx[1]);

    thread.reset(new QThread(kwinApp()));
    connection->moveToThread(thread.get());
    thread->start();

    connection->establishConnection();
    QVERIFY(connectedSpy.count() || connectedSpy.wait());
    QCOMPARE(connectedSpy.count(), 1);
    QVERIFY(connection->established());

    queue.reset(new Clt::EventQueue);
    queue->setup(connection);
    QVERIFY(queue->isValid());

    registry.reset(new Clt::Registry);
    registry->setEventQueue(queue.get());

    connect_outputs();

    QSignalSpy allAnnounced(registry.get(), &Clt::Registry::interfacesAnnounced);
    QVERIFY(allAnnounced.isValid());

    registry->create(connection);
    QVERIFY(registry->isValid());

    registry->setup();
    QVERIFY(allAnnounced.count() || allAnnounced.wait());
    QCOMPARE(allAnnounced.count(), 1);

    interfaces.compositor.reset(registry->createCompositor(
        registry->interface(Clt::Registry::Interface::Compositor).name,
        registry->interface(Clt::Registry::Interface::Compositor).version));
    QVERIFY(interfaces.compositor->isValid());

    interfaces.subCompositor.reset(registry->createSubCompositor(
        registry->interface(Clt::Registry::Interface::SubCompositor).name,
        registry->interface(Clt::Registry::Interface::SubCompositor).version));
    QVERIFY(interfaces.subCompositor->isValid());

    interfaces.shm.reset(
        registry->createShmPool(registry->interface(Clt::Registry::Interface::Shm).name,
                                registry->interface(Clt::Registry::Interface::Shm).version));
    QVERIFY(interfaces.shm->isValid());

    interfaces.xdg_shell.reset(
        registry->createXdgShell(registry->interface(Clt::Registry::Interface::XdgShell).name,
                                 registry->interface(Clt::Registry::Interface::XdgShell).version));
    QVERIFY(interfaces.xdg_shell->isValid());

    interfaces.layer_shell.reset(registry->createLayerShellV1(
        registry->interface(Clt::Registry::Interface::LayerShellV1).name,
        registry->interface(Clt::Registry::Interface::LayerShellV1).version));
    QVERIFY(interfaces.layer_shell->isValid());

    if (flags.testFlag(AdditionalWaylandInterface::Seat)) {
        interfaces.seat.reset(
            registry->createSeat(registry->interface(Clt::Registry::Interface::Seat).name,
                                 registry->interface(Clt::Registry::Interface::Seat).version));
        QVERIFY(interfaces.seat->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::ShadowManager)) {
        interfaces.shadowManager.reset(registry->createShadowManager(
            registry->interface(Clt::Registry::Interface::Shadow).name,
            registry->interface(Clt::Registry::Interface::Shadow).version));
        QVERIFY(interfaces.shadowManager->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::PlasmaShell)) {
        interfaces.plasmaShell.reset(registry->createPlasmaShell(
            registry->interface(Clt::Registry::Interface::PlasmaShell).name,
            registry->interface(Clt::Registry::Interface::PlasmaShell).version));
        QVERIFY(interfaces.plasmaShell->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::WindowManagement)) {
        interfaces.windowManagement.reset(registry->createPlasmaWindowManagement(
            registry->interface(Clt::Registry::Interface::PlasmaWindowManagement).name,
            registry->interface(Clt::Registry::Interface::PlasmaWindowManagement).version));
        QVERIFY(interfaces.windowManagement->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::PointerConstraints)) {
        interfaces.pointerConstraints.reset(registry->createPointerConstraints(
            registry->interface(Clt::Registry::Interface::PointerConstraintsUnstableV1).name,
            registry->interface(Clt::Registry::Interface::PointerConstraintsUnstableV1).version));
        QVERIFY(interfaces.pointerConstraints->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::IdleInhibition)) {
        interfaces.idleInhibit.reset(registry->createIdleInhibitManager(
            registry->interface(Clt::Registry::Interface::IdleInhibitManagerUnstableV1).name,
            registry->interface(Clt::Registry::Interface::IdleInhibitManagerUnstableV1).version));
        QVERIFY(interfaces.idleInhibit->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::AppMenu)) {
        interfaces.appMenu.reset(registry->createAppMenuManager(
            registry->interface(Clt::Registry::Interface::AppMenu).name,
            registry->interface(Clt::Registry::Interface::AppMenu).version));
        QVERIFY(interfaces.appMenu->isValid());
    }

    if (flags.testFlag(AdditionalWaylandInterface::XdgDecoration)) {
        interfaces.xdgDecoration.reset(registry->createXdgDecorationManager(
            registry->interface(Clt::Registry::Interface::XdgDecorationUnstableV1).name,
            registry->interface(Clt::Registry::Interface::XdgDecorationUnstableV1).version));
        QVERIFY(interfaces.xdgDecoration->isValid());
    }
}

client::client(client&& other) noexcept
{
    *this = std::move(other);
}

client& client::operator=(client&& other) noexcept
{
    cleanup();

    QObject::disconnect(other.output_announced);
    for (auto& con : other.output_removals) {
        QObject::disconnect(con);
    }

    connection = other.connection;
    other.connection = nullptr;

    thread = std::move(other.thread);
    queue = std::move(other.queue);
    registry = std::move(other.registry);
    interfaces = std::move(other.interfaces);

    connect_outputs();

    return *this;
}

client::~client()
{
    cleanup();
}

void client::connect_outputs()
{
    output_announced = QObject::connect(
        registry.get(), &Clt::Registry::outputAnnounced, [&](quint32 name, quint32 version) {
            auto output = std::unique_ptr<Clt::Output>(
                registry->createOutput(name, version, registry.get()));
            output_removals.push_back(output_removal_connection(output.get()));
            interfaces.outputs.push_back(std::move(output));
        });
    for (auto& output : interfaces.outputs) {
        output_removals.push_back(output_removal_connection(output.get()));
    }
}

QMetaObject::Connection client::output_removal_connection(Wrapland::Client::Output* output)
{
    return QObject::connect(output, &Clt::Output::removed, [output, this]() {
        output->deleteLater();
        auto& outs = interfaces.outputs;
        outs.erase(std::remove_if(outs.begin(),
                                  outs.end(),
                                  [output](auto const& out) { return out.get() == output; }),
                   outs.end());
    });
}

void client::cleanup()
{
    if (!connection) {
        return;
    }
    interfaces = {};
    registry.reset();
    queue.reset();

    if (thread) {
        QSignalSpy spy(connection, &QObject::destroyed);
        QVERIFY(spy.isValid());

        connection->deleteLater();
        QVERIFY(!spy.isEmpty() || spy.wait());
        QCOMPARE(spy.count(), 1);

        thread->quit();
        thread->wait();
        thread.reset();
        connection = nullptr;
    } else {
        delete connection;
        connection = nullptr;
    }
}

void setupWaylandConnection(AdditionalWaylandInterfaces flags)
{
    QVERIFY(!s_waylandConnection.connection);
    s_waylandConnection = std::move(client(flags));
}

void destroyWaylandConnection()
{
    s_waylandConnection = client();
}

Clt::ConnectionThread* waylandConnection()
{
    return s_waylandConnection.connection;
}

Clt::Compositor* waylandCompositor()
{
    return s_waylandConnection.interfaces.compositor.get();
}

Clt::SubCompositor* waylandSubCompositor()
{
    return s_waylandConnection.interfaces.subCompositor.get();
}

Clt::ShadowManager* waylandShadowManager()
{
    return s_waylandConnection.interfaces.shadowManager.get();
}

Clt::ShmPool* waylandShmPool()
{
    return s_waylandConnection.interfaces.shm.get();
}

Clt::Seat* waylandSeat()
{
    return s_waylandConnection.interfaces.seat.get();
}

Clt::PlasmaShell* waylandPlasmaShell()
{
    return s_waylandConnection.interfaces.plasmaShell.get();
}

Clt::PlasmaWindowManagement* waylandWindowManagement()
{
    return s_waylandConnection.interfaces.windowManagement.get();
}

Clt::PointerConstraints* waylandPointerConstraints()
{
    return s_waylandConnection.interfaces.pointerConstraints.get();
}

Clt::IdleInhibitManager* waylandIdleInhibitManager()
{
    return s_waylandConnection.interfaces.idleInhibit.get();
}

Clt::AppMenuManager* waylandAppMenuManager()
{
    return s_waylandConnection.interfaces.appMenu.get();
}

Clt::XdgDecorationManager* xdgDecorationManager()
{
    return s_waylandConnection.interfaces.xdgDecoration.get();
}

Clt::LayerShellV1* layer_shell()
{
    return s_waylandConnection.interfaces.layer_shell.get();
}

std::vector<Clt::Output*> outputs()
{
    std::vector<Clt::Output*> ret;
    for (auto& output : s_waylandConnection.interfaces.outputs) {
        ret.push_back(output.get());
    }
    return ret;
}

bool waitForWaylandPointer()
{
    if (!s_waylandConnection.interfaces.seat) {
        return false;
    }
    QSignalSpy hasPointerSpy(s_waylandConnection.interfaces.seat.get(),
                             &Clt::Seat::hasPointerChanged);
    if (!hasPointerSpy.isValid()) {
        return false;
    }
    return hasPointerSpy.wait();
}

bool waitForWaylandTouch()
{
    if (!s_waylandConnection.interfaces.seat) {
        return false;
    }
    QSignalSpy hasTouchSpy(s_waylandConnection.interfaces.seat.get(), &Clt::Seat::hasTouchChanged);
    if (!hasTouchSpy.isValid()) {
        return false;
    }
    return hasTouchSpy.wait();
}

bool waitForWaylandKeyboard()
{
    if (!s_waylandConnection.interfaces.seat) {
        return false;
    }
    QSignalSpy hasKeyboardSpy(s_waylandConnection.interfaces.seat.get(),
                              &Clt::Seat::hasKeyboardChanged);
    if (!hasKeyboardSpy.isValid()) {
        return false;
    }
    return hasKeyboardSpy.wait();
}

void render(Clt::Surface* surface,
            const QSize& size,
            const QColor& color,
            const QImage::Format& format)
{
    QImage img(size, format);
    img.fill(color);
    render(surface, img);
}

void render(Clt::Surface* surface, const QImage& img)
{
    surface->attachBuffer(s_waylandConnection.interfaces.shm->createBuffer(img));
    surface->damage(QRect(QPoint(0, 0), img.size()));
    surface->commit(Clt::Surface::CommitFlag::None);
}

win::wayland::window* waitForWaylandWindowShown(int timeout)
{
    QSignalSpy clientAddedSpy(waylandServer(), &WaylandServer::window_added);
    if (!clientAddedSpy.isValid()) {
        return nullptr;
    }
    if (!clientAddedSpy.wait(timeout)) {
        return nullptr;
    }
    return clientAddedSpy.first().first().value<win::wayland::window*>();
}

win::wayland::window* renderAndWaitForShown(Clt::Surface* surface,
                                            const QSize& size,
                                            const QColor& color,
                                            const QImage::Format& format,
                                            int timeout)
{
    QSignalSpy clientAddedSpy(waylandServer(), &WaylandServer::window_added);
    if (!clientAddedSpy.isValid()) {
        return nullptr;
    }
    render(surface, size, color, format);
    flushWaylandConnection();
    if (!clientAddedSpy.wait(timeout)) {
        return nullptr;
    }
    return clientAddedSpy.first().first().value<win::wayland::window*>();
}

void flushWaylandConnection()
{
    if (s_waylandConnection.connection) {
        s_waylandConnection.connection->flush();
    }
}

Clt::Surface* createSurface(QObject* parent)
{
    if (!s_waylandConnection.interfaces.compositor) {
        return nullptr;
    }
    auto s = s_waylandConnection.interfaces.compositor->createSurface(parent);
    if (!s->isValid()) {
        delete s;
        return nullptr;
    }
    return s;
}

Clt::SubSurface*
createSubSurface(Clt::Surface* surface, Clt::Surface* parentSurface, QObject* parent)
{
    if (!s_waylandConnection.interfaces.subCompositor) {
        return nullptr;
    }
    auto s = s_waylandConnection.interfaces.subCompositor->createSubSurface(
        surface, parentSurface, parent);
    if (!s->isValid()) {
        delete s;
        return nullptr;
    }
    return s;
}

Clt::XdgShellToplevel*
create_xdg_shell_toplevel(Clt::Surface* surface, QObject* parent, CreationSetup creationSetup)
{
    if (!s_waylandConnection.interfaces.xdg_shell) {
        return nullptr;
    }
    auto s = s_waylandConnection.interfaces.xdg_shell->create_toplevel(surface, parent);
    if (!s->isValid()) {
        delete s;
        return nullptr;
    }
    if (creationSetup == CreationSetup::CreateAndConfigure) {
        init_xdg_shell_toplevel(surface, s);
    }
    return s;
}

Clt::XdgShellPopup* create_xdg_shell_popup(Clt::Surface* surface,
                                           Clt::XdgShellToplevel* parentSurface,
                                           const Clt::XdgPositioner& positioner,
                                           QObject* parent,
                                           CreationSetup creationSetup)
{
    if (!s_waylandConnection.interfaces.xdg_shell) {
        return nullptr;
    }
    auto s = s_waylandConnection.interfaces.xdg_shell->create_popup(
        surface, parentSurface, positioner, parent);
    if (!s->isValid()) {
        delete s;
        return nullptr;
    }
    if (creationSetup == CreationSetup::CreateAndConfigure) {
        init_xdg_shell_popup(surface, s);
    }
    return s;
}

void init_xdg_shell_toplevel(Clt::Surface* surface, Clt::XdgShellToplevel* shellSurface)
{
    // wait for configure
    QSignalSpy configureRequestedSpy(shellSurface, &Clt::XdgShellToplevel::configureRequested);
    QVERIFY(configureRequestedSpy.isValid());
    surface->commit(Clt::Surface::CommitFlag::None);
    QVERIFY(configureRequestedSpy.wait());
    shellSurface->ackConfigure(configureRequestedSpy.last()[2].toInt());
}

void init_xdg_shell_popup(Clt::Surface* surface, Clt::XdgShellPopup* shellPopup)
{
    // wait for configure
    QSignalSpy configureRequestedSpy(shellPopup, &Clt::XdgShellPopup::configureRequested);
    QVERIFY(configureRequestedSpy.isValid());
    surface->commit(Clt::Surface::CommitFlag::None);
    QVERIFY(configureRequestedSpy.wait());
    shellPopup->ackConfigure(configureRequestedSpy.last()[1].toInt());
}

bool waitForWindowDestroyed(Toplevel* window)
{
    QSignalSpy destroyedSpy(window, &QObject::destroyed);
    if (!destroyedSpy.isValid()) {
        return false;
    }
    return destroyedSpy.wait();
}

void lockScreen()
{
    QVERIFY(!waylandServer()->isScreenLocked());

    QSignalSpy lockStateChangedSpy(ScreenLocker::KSldApp::self(),
                                   &ScreenLocker::KSldApp::lockStateChanged);
    QVERIFY(lockStateChangedSpy.isValid());
    QSignalSpy lockWatcherSpy(ScreenLockerWatcher::self(), &ScreenLockerWatcher::locked);
    QVERIFY(lockWatcherSpy.isValid());

    ScreenLocker::KSldApp::self()->lock(ScreenLocker::EstablishLock::Immediate);
    QCOMPARE(lockStateChangedSpy.count(), 1);

    QVERIFY(waylandServer()->isScreenLocked());
    QVERIFY(lockWatcherSpy.wait());
    QCOMPARE(lockWatcherSpy.count(), 1);
    QCOMPARE(lockStateChangedSpy.count(), 2);

    QVERIFY(ScreenLockerWatcher::self()->isLocked());
}

void unlockScreen()
{
    QSignalSpy lockStateChangedSpy(ScreenLocker::KSldApp::self(),
                                   &ScreenLocker::KSldApp::lockStateChanged);
    QVERIFY(lockStateChangedSpy.isValid());
    QSignalSpy lockWatcherSpy(ScreenLockerWatcher::self(), &ScreenLockerWatcher::locked);
    QVERIFY(lockWatcherSpy.isValid());

    auto const children = ScreenLocker::KSldApp::self()->children();
    auto logind_integration_found{false};
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (qstrcmp((*it)->metaObject()->className(), "LogindIntegration") == 0) {
            logind_integration_found = true;

            // KScreenLocker does not handle unlock requests via logind reliable as it sends a
            // SIGTERM signal to the lock process which sometimes under high system load is not
            // received and handled by the process.
            // It is unclear why the signal is never received but we can repeat sending the signal
            // mutliple times (here 10) assuming that after few tries one of them will be received.
            int unlock_tries = 0;
            while (unlock_tries++ < 10) {
                QMetaObject::invokeMethod(*it, "requestUnlock");
                lockWatcherSpy.wait(1000);
                if (lockWatcherSpy.count()) {
                    break;
                }
            }

            break;
        }
    }
    QVERIFY(logind_integration_found);
    QCOMPARE(lockWatcherSpy.count(), 1);
    QCOMPARE(lockStateChangedSpy.count(), 1);

    QVERIFY(!waylandServer()->isScreenLocked());

    QVERIFY(!ScreenLockerWatcher::self()->isLocked());
}

}
}

/*
    SPDX-FileCopyrightText: 1999, 2000 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "main_x11.h"

#include "main.h"
#include <config-kwin.h>

#include "base/options.h"
#include "base/seat/backend/logind/session.h"
#include "base/x11/selection_owner.h"
#include "base/x11/xcb/helpers.h"
#include "input/x11/platform.h"
#include "input/x11/redirect.h"
#include "render/shortcuts_init.h"
#include "script/platform.h"
#include "win/shortcuts_init.h"
#include "win/x11/space.h"
#include "win/x11/space_event.h"
#include "win/x11/xcb_event_filter.h"
#include <desktop/kde/platform.h>

#include <KConfigGroup>
#include <KCrash>
#include <KLocalizedString>
#include <KPluginMetaData>
#include <KSignalHandler>

#include <QCommandLineParser>
#include <QFile>
#include <QSurfaceFormat>
#include <QtDBus>
#include <QtGui/private/qtx11extras_p.h>
#include <qplatformdefs.h>

// system
#include <iostream>
#include <unistd.h>

Q_LOGGING_CATEGORY(KWIN_CORE, "kwin_core", QtWarningMsg)

constexpr char kwin_internal_name[]{"kwin_x11"};

namespace KWin
{

class KWinSelectionOwner : public base::x11::selection_owner
{
public:
    KWinSelectionOwner(xcb_connection_t* con, int screen)
        : selection_owner(make_selection_atom(con, screen), screen)
        , con{con}
    {
    }

private:
    bool genericReply(xcb_atom_t target_P, xcb_atom_t property_P, xcb_window_t requestor_P) override
    {
        if (target_P == xa_version) {
            int32_t version[] = {2, 0};
            xcb_change_property(con,
                                XCB_PROP_MODE_REPLACE,
                                requestor_P,
                                property_P,
                                XCB_ATOM_INTEGER,
                                32,
                                2,
                                version);
        } else
            return selection_owner::genericReply(target_P, property_P, requestor_P);
        return true;
    }

    void replyTargets(xcb_atom_t property_P, xcb_window_t requestor_P) override
    {
        selection_owner::replyTargets(property_P, requestor_P);
        xcb_atom_t atoms[1] = {xa_version};
        // PropModeAppend !
        xcb_change_property(
            con, XCB_PROP_MODE_APPEND, requestor_P, property_P, XCB_ATOM_ATOM, 32, 1, atoms);
    }

    void getAtoms() override
    {
        selection_owner::getAtoms();
        if (xa_version == XCB_ATOM_NONE) {
            const QByteArray name(QByteArrayLiteral("VERSION"));
            unique_cptr<xcb_intern_atom_reply_t> atom(xcb_intern_atom_reply(
                con,
                xcb_intern_atom_unchecked(con, false, name.length(), name.constData()),
                nullptr));
            if (atom) {
                xa_version = atom->atom;
            }
        }
    }

    xcb_atom_t make_selection_atom(xcb_connection_t* con, int screen_P)
    {
        if (screen_P < 0)
            screen_P = QX11Info::appScreen();
        QByteArray screen(QByteArrayLiteral("WM_S"));
        screen.append(QByteArray::number(screen_P));
        unique_cptr<xcb_intern_atom_reply_t> atom(xcb_intern_atom_reply(
            con,
            xcb_intern_atom_unchecked(con, false, screen.length(), screen.constData()),
            nullptr));
        if (!atom) {
            return XCB_ATOM_NONE;
        }
        return atom->atom;
    }
    static xcb_atom_t xa_version;
    xcb_connection_t* con;
};
xcb_atom_t KWinSelectionOwner::xa_version = XCB_ATOM_NONE;

//************************************
// ApplicationX11
//************************************

int ApplicationX11::crashes = 0;

ApplicationX11::ApplicationX11(int& argc, char** argv)
    : QApplication(argc, argv)
    , base{base::config(KConfig::OpenFlag::FullConfig, "kwinrc")}
    , m_replace(false)
{
    app_init();

    base.x11_data.connection = QX11Info::connection();
    base.x11_data.root_window = QX11Info::appRootWindow();
}

ApplicationX11::~ApplicationX11()
{
    base.mod.space.reset();
    if (owner && owner->ownerWindow() != XCB_WINDOW_NONE) {
        // If there was no --replace (no new WM)
        xcb_set_input_focus(base.x11_data.connection,
                            XCB_INPUT_FOCUS_POINTER_ROOT,
                            XCB_INPUT_FOCUS_POINTER_ROOT,
                            base.x11_data.time);
    }
}

void ApplicationX11::setReplace(bool replace)
{
    m_replace = replace;
}

void ApplicationX11::lostSelection()
{
    sendPostedEvents();
    event_filter.reset();
    base.mod.space.reset();
    base.mod.render.reset();

    // Remove windowmanager privileges
    base::x11::xcb::select_input(
        base.x11_data.connection, base.x11_data.root_window, XCB_EVENT_MASK_PROPERTY_CHANGE);
    quit();
}

void ApplicationX11::start()
{
    setQuitOnLastWindowClosed(false);
    setQuitLockEnabled(false);

    base.is_crash_restart = crashes > 0;

    crashChecking();
    base.x11_data.screen_number = QX11Info::appScreen();
    base::x11::xcb::extensions::create(base.x11_data);

    owner = std::make_unique<KWinSelectionOwner>(base.x11_data.connection,
                                                 base.x11_data.screen_number);
    QObject::connect(owner.get(), &KWinSelectionOwner::failedToClaimOwnership, [] {
        fputs(i18n("kwin: unable to claim manager selection, another wm running? (try using "
                   "--replace)\n")
                  .toLocal8Bit()
                  .constData(),
              stderr);
        ::exit(1);
    });
    QObject::connect(
        owner.get(), &KWinSelectionOwner::lostOwnership, this, &ApplicationX11::lostSelection);
    QObject::connect(owner.get(), &KWinSelectionOwner::claimedOwnership, [this] {
        base.options = base::create_options(base::operation_mode::x11, base.config.main);

        // Check  whether another windowmanager is running
        const uint32_t maskValues[] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT};
        unique_cptr<xcb_generic_error_t> redirectCheck(
            xcb_request_check(base.x11_data.connection,
                              xcb_change_window_attributes_checked(base.x11_data.connection,
                                                                   base.x11_data.root_window,
                                                                   XCB_CW_EVENT_MASK,
                                                                   maskValues)));
        if (redirectCheck) {
            fputs(i18n("kwin: another window manager is running (try using --replace)\n")
                      .toLocal8Bit()
                      .constData(),
                  stderr);
            // If this is a crash-restart, DrKonqi may have stopped the process w/o killing the
            // connection.
            if (crashes == 0) {
                ::exit(1);
            }
        }

        base.session = std::make_unique<base::seat::backend::logind::session>();
        base.mod.render = std::make_unique<render::backend::x11::platform<base_t>>(base);
        base.mod.input = std::make_unique<input::x11::platform<base_t>>(base);

        base.update_outputs();
        auto render = static_cast<render::backend::x11::platform<base_t>*>(base.mod.render.get());
        try {
            render->init();
        } catch (std::exception const&) {
            std::cerr << "FATAL ERROR: backend failed to initialize, exiting now" << std::endl;
            ::exit(1);
        }

        try {
            base.mod.space = std::make_unique<base_t::space_t>(*base.mod.render, *base.mod.input);
        } catch (std::exception& ex) {
            qCCritical(KWIN_CORE) << "Abort since space creation fails with:" << ex.what();
            exit(1);
        }

        base.mod.space->mod.desktop
            = std::make_unique<desktop::kde::platform<base_t::space_t>>(*base.mod.space);
        win::init_shortcuts(*base.mod.space);
        render::init_shortcuts(*base.mod.render);

        event_filter
            = std::make_unique<win::x11::xcb_event_filter<base_t::space_t>>(*base.mod.space);
        installNativeEventFilter(event_filter.get());

        base.mod.script = std::make_unique<scripting::platform<base_t::space_t>>(*base.mod.space);
        render->start(*base.mod.space);

        // Trigger possible errors, there's still a chance to abort.
        base::x11::xcb::sync(base.x11_data.connection);
        notifyKSplash();
    });

    // we need to do an XSync here, otherwise the QPA might crash us later on
    base::x11::xcb::sync(base.x11_data.connection);
    owner->claim(m_replace || crashes > 0, true);
}

void ApplicationX11::crashChecking()
{
    KCrash::setEmergencySaveFunction(ApplicationX11::crashHandler);

    if (crashes >= 4) {
        // Something has gone seriously wrong
        qCDebug(KWIN_CORE) << "More than 3 crashes recently. Exiting now.";
        ::exit(1);
    }

    if (crashes >= 2) {
        // Disable compositing if we have had too many crashes
        qCDebug(KWIN_CORE) << "More than 1 crash recently. Disabling compositing.";
        KConfigGroup compgroup(KSharedConfig::openConfig(), "Compositing");
        compgroup.writeEntry("Enabled", false);
    }

    // Reset crashes count if we stay up for more that 15 seconds
    QTimer::singleShot(15 * 1000, this, [] { crashes = 0; });
}

void ApplicationX11::notifyKSplash()
{
    // Tell KSplash that KWin has started
    QDBusMessage ksplashProgressMessage
        = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                         QStringLiteral("/KSplash"),
                                         QStringLiteral("org.kde.KSplash"),
                                         QStringLiteral("setStage"));
    ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("wm"));
    QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
}

void ApplicationX11::crashHandler(int signal)
{
    crashes++;

    fprintf(stderr,
            "Application::crashHandler() called with signal %d; recent crashes: %d\n",
            signal,
            crashes);
    char cmd[1024];
    sprintf(cmd,
            "%s --crashes %d &",
            QFile::encodeName(QCoreApplication::applicationFilePath()).constData(),
            crashes);

    sleep(1);
    system(cmd);
}

} // namespace

int main(int argc, char* argv[])
{
    KLocalizedString::setApplicationDomain("kwin");

    int primaryScreen = 0;
    xcb_connection_t* c = xcb_connect(nullptr, &primaryScreen);
    if (!c || xcb_connection_has_error(c)) {
        fprintf(stderr,
                "%s: FATAL ERROR while trying to open display %s\n",
                argv[0],
                qgetenv("DISPLAY").constData());
        exit(1);
    }

    xcb_disconnect(c);
    c = nullptr;

    signal(SIGPIPE, SIG_IGN);

    // enforce xcb plugin, unfortunately command line switch has precedence
    setenv("QT_QPA_PLATFORM", "xcb", true);

    // disable highdpi scaling
    setenv("QT_ENABLE_HIGHDPI_SCALING", "0", true);

    qunsetenv("QT_DEVICE_PIXEL_RATIO");
    qunsetenv("QT_SCALE_FACTOR");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");

    // KSMServer talks to us directly on DBus.
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    // For sharing thumbnails between our scene graph and qtquick.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // shared opengl contexts must have the same reset notification policy
    format.setOptions(QSurfaceFormat::ResetNotification);
    // disables vsync for any QtQuick windows we create (BUG 406180)
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    KWin::ApplicationX11 a(argc, argv);

    // reset QT_QPA_PLATFORM so we don't propagate it to our children (e.g. apps launched from the
    // overview effect)
    qunsetenv("QT_QPA_PLATFORM");
    qunsetenv("QT_ENABLE_HIGHDPI_SCALING");

    KSignalHandler::self()->watchSignal(SIGTERM);
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGHUP);
    QObject::connect(
        KSignalHandler::self(), &KSignalHandler::signalReceived, &a, &QCoreApplication::exit);

    KWin::app_create_about_data();

    QCommandLineOption crashesOption(
        "crashes", i18n("Indicate that KWin has recently crashed n times"), QStringLiteral("n"));
    QCommandLineOption replaceOption(
        QStringLiteral("replace"),
        i18n("Replace already-running ICCCM2.0-compliant window manager"));

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("KWinFT X11 Window Manager"));
    KAboutData::applicationData().setupCommandLine(&parser);

    parser.addOption(crashesOption);
    parser.addOption(replaceOption);

    parser.process(a);

    qDebug("Starting KWinFT (X11) %s", KWIN_VERSION_STRING);

    KAboutData::applicationData().processCommandLine(&parser);
    a.crashes = parser.value("crashes").toInt();

    a.setReplace(parser.isSet(replaceOption));

    // perform sanity checks
    if (a.platformName().toLower() != QStringLiteral("xcb")) {
        fprintf(stderr,
                "%s: FATAL ERROR expecting platform xcb but got platform %s\n",
                argv[0],
                qPrintable(a.platformName()));
        exit(1);
    }
    if (!QX11Info::display()) {
        fprintf(stderr,
                "%s: FATAL ERROR KWin requires Xlib support in the xcb plugin. Do not configure Qt "
                "with -no-xcb-xlib\n",
                argv[0]);
        exit(1);
    }

    a.start();

    return a.exec();
}

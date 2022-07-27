/*
    SPDX-FileCopyrightText: 1999, 2000 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2012 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2022 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "kwin_export.h"
#include "kwinglobals.h"
#include "win/types.h"

#include <KConfigWatcher>

namespace KWin::base
{

// Whether to keep all windows mapped when compositing (i.e. whether to have
// actively updated window pixmaps).
enum HiddenPreviews {
    // The normal mode with regard to mapped windows. Hidden (minimized, etc.)
    // and windows on inactive virtual desktops are not mapped, their pixmaps
    // are only their icons.
    HiddenPreviewsNever,
    // Like normal mode, but shown windows (i.e. on inactive virtual desktops)
    // are kept mapped, only hidden windows are unmapped.
    HiddenPreviewsShown,
    // All windows are kept mapped regardless of their state.
    HiddenPreviewsAlways
};

KWIN_EXPORT OpenGLPlatformInterface defaultGlPlatformInterface();

class Settings;

class KWIN_EXPORT options_qobject : public QObject
{
    Q_OBJECT
    Q_ENUMS(FocusPolicy)
    Q_ENUMS(MouseCommand)
    Q_ENUMS(MouseWheelCommand)
    Q_ENUMS(WindowOperation)
    Q_ENUMS(AnimationCurve)

    Q_PROPERTY(
        FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy NOTIFY focusPolicyChanged)
    Q_PROPERTY(AnimationCurve animationCurve READ animationCurve WRITE setAnimationCurve NOTIFY
                   animationCurveChanged)
    Q_PROPERTY(bool nextFocusPrefersMouse READ isNextFocusPrefersMouse WRITE
                   setNextFocusPrefersMouse NOTIFY nextFocusPrefersMouseChanged)
    /**
     * Whether clicking on a window raises it in FocusFollowsMouse
     * mode or not.
     */
    Q_PROPERTY(bool clickRaise READ isClickRaise WRITE setClickRaise NOTIFY clickRaiseChanged)
    /**
     * Whether autoraise is enabled FocusFollowsMouse mode or not.
     */
    Q_PROPERTY(bool autoRaise READ isAutoRaise WRITE setAutoRaise NOTIFY autoRaiseChanged)
    /**
     * Autoraise interval.
     */
    Q_PROPERTY(int autoRaiseInterval READ autoRaiseInterval WRITE setAutoRaiseInterval NOTIFY
                   autoRaiseIntervalChanged)
    /**
     * Delayed focus interval.
     */
    Q_PROPERTY(int delayFocusInterval READ delayFocusInterval WRITE setDelayFocusInterval NOTIFY
                   delayFocusIntervalChanged)
    /**
     * Whether to see Xinerama screens separately for focus (in Alt+Tab, when activating next
     * client)
     */
    Q_PROPERTY(bool separateScreenFocus READ isSeparateScreenFocus WRITE setSeparateScreenFocus
                   NOTIFY separateScreenFocusChanged)
    Q_PROPERTY(
        KWin::win::placement placement READ placement WRITE setPlacement NOTIFY placementChanged)
    Q_PROPERTY(bool focusPolicyIsReasonable READ focusPolicyIsReasonable NOTIFY
                   focusPolicyIsResonableChanged)
    /**
     * The size of the zone that triggers snapping on desktop borders.
     */
    Q_PROPERTY(
        int borderSnapZone READ borderSnapZone WRITE setBorderSnapZone NOTIFY borderSnapZoneChanged)
    /**
     * The size of the zone that triggers snapping with other windows.
     */
    Q_PROPERTY(
        int windowSnapZone READ windowSnapZone WRITE setWindowSnapZone NOTIFY windowSnapZoneChanged)
    /**
     * The size of the zone that triggers snapping on the screen center.
     */
    Q_PROPERTY(
        int centerSnapZone READ centerSnapZone WRITE setCenterSnapZone NOTIFY centerSnapZoneChanged)
    /**
     * Snap only when windows will overlap.
     */
    Q_PROPERTY(bool snapOnlyWhenOverlapping READ isSnapOnlyWhenOverlapping WRITE
                   setSnapOnlyWhenOverlapping NOTIFY snapOnlyWhenOverlappingChanged)
    /**
     * Whether or not we roll over to the other edge when switching desktops past the edge.
     */
    Q_PROPERTY(bool rollOverDesktops READ isRollOverDesktops WRITE setRollOverDesktops NOTIFY
                   rollOverDesktopsChanged)
    /**
     * 0 - 4 , see Workspace::allowClientActivation()
     */
    Q_PROPERTY(win::fsp_level focusStealingPreventionLevel READ focusStealingPreventionLevel WRITE
                   setFocusStealingPreventionLevel NOTIFY focusStealingPreventionLevelChanged)
    Q_PROPERTY(KWin::base::options_qobject::WindowOperation operationTitlebarDblClick READ
                   operationTitlebarDblClick WRITE setOperationTitlebarDblClick NOTIFY
                       operationTitlebarDblClickChanged)
    Q_PROPERTY(KWin::base::options_qobject::WindowOperation operationMaxButtonLeftClick READ
                   operationMaxButtonLeftClick WRITE setOperationMaxButtonLeftClick NOTIFY
                       operationMaxButtonLeftClickChanged)
    Q_PROPERTY(KWin::base::options_qobject::WindowOperation operationMaxButtonMiddleClick READ
                   operationMaxButtonMiddleClick WRITE setOperationMaxButtonMiddleClick NOTIFY
                       operationMaxButtonMiddleClickChanged)
    Q_PROPERTY(KWin::base::options_qobject::WindowOperation operationMaxButtonRightClick READ
                   operationMaxButtonRightClick WRITE setOperationMaxButtonRightClick NOTIFY
                       operationMaxButtonRightClickChanged)
    Q_PROPERTY(MouseCommand commandActiveTitlebar1 READ commandActiveTitlebar1 WRITE
                   setCommandActiveTitlebar1 NOTIFY commandActiveTitlebar1Changed)
    Q_PROPERTY(MouseCommand commandActiveTitlebar2 READ commandActiveTitlebar2 WRITE
                   setCommandActiveTitlebar2 NOTIFY commandActiveTitlebar2Changed)
    Q_PROPERTY(MouseCommand commandActiveTitlebar3 READ commandActiveTitlebar3 WRITE
                   setCommandActiveTitlebar3 NOTIFY commandActiveTitlebar3Changed)
    Q_PROPERTY(MouseCommand commandInactiveTitlebar1 READ commandInactiveTitlebar1 WRITE
                   setCommandInactiveTitlebar1 NOTIFY commandInactiveTitlebar1Changed)
    Q_PROPERTY(MouseCommand commandInactiveTitlebar2 READ commandInactiveTitlebar2 WRITE
                   setCommandInactiveTitlebar2 NOTIFY commandInactiveTitlebar2Changed)
    Q_PROPERTY(MouseCommand commandInactiveTitlebar3 READ commandInactiveTitlebar3 WRITE
                   setCommandInactiveTitlebar3 NOTIFY commandInactiveTitlebar3Changed)
    Q_PROPERTY(MouseCommand commandWindow1 READ commandWindow1 WRITE setCommandWindow1 NOTIFY
                   commandWindow1Changed)
    Q_PROPERTY(MouseCommand commandWindow2 READ commandWindow2 WRITE setCommandWindow2 NOTIFY
                   commandWindow2Changed)
    Q_PROPERTY(MouseCommand commandWindow3 READ commandWindow3 WRITE setCommandWindow3 NOTIFY
                   commandWindow3Changed)
    Q_PROPERTY(MouseCommand commandWindowWheel READ commandWindowWheel WRITE setCommandWindowWheel
                   NOTIFY commandWindowWheelChanged)
    Q_PROPERTY(
        MouseCommand commandAll1 READ commandAll1 WRITE setCommandAll1 NOTIFY commandAll1Changed)
    Q_PROPERTY(
        MouseCommand commandAll2 READ commandAll2 WRITE setCommandAll2 NOTIFY commandAll2Changed)
    Q_PROPERTY(
        MouseCommand commandAll3 READ commandAll3 WRITE setCommandAll3 NOTIFY commandAll3Changed)
    Q_PROPERTY(uint keyCmdAllModKey READ keyCmdAllModKey WRITE setKeyCmdAllModKey NOTIFY
                   keyCmdAllModKeyChanged)
    /**
     * Whether the visible name should be condensed.
     */
    Q_PROPERTY(bool condensedTitle READ condensedTitle WRITE setCondensedTitle NOTIFY
                   condensedTitleChanged)
    /**
     * Whether a window gets maximized when it reaches top screen edge while being moved.
     */
    Q_PROPERTY(bool electricBorderMaximize READ electricBorderMaximize WRITE
                   setElectricBorderMaximize NOTIFY electricBorderMaximizeChanged)
    /**
     * Whether a window is tiled to half screen when reaching left or right screen edge while been
     * moved.
     */
    Q_PROPERTY(bool electricBorderTiling READ electricBorderTiling WRITE setElectricBorderTiling
                   NOTIFY electricBorderTilingChanged)
    /**
     * Whether a window is tiled to half screen when reaching left or right screen edge while been
     * moved.
     */
    Q_PROPERTY(float electricBorderCornerRatio READ electricBorderCornerRatio WRITE
                   setElectricBorderCornerRatio NOTIFY electricBorderCornerRatioChanged)
    Q_PROPERTY(bool borderlessMaximizedWindows READ borderlessMaximizedWindows WRITE
                   setBorderlessMaximizedWindows NOTIFY borderlessMaximizedWindowsChanged)
    /**
     * timeout before non-responding application will be killed after attempt to close.
     */
    Q_PROPERTY(int killPingTimeout READ killPingTimeout WRITE setKillPingTimeout NOTIFY
                   killPingTimeoutChanged)
    /**
     * Whether to hide utility windows for inactive applications.
     */
    Q_PROPERTY(bool hideUtilityWindowsForInactive READ isHideUtilityWindowsForInactive WRITE
                   setHideUtilityWindowsForInactive NOTIFY hideUtilityWindowsForInactiveChanged)
    Q_PROPERTY(int compositingMode READ compositingMode WRITE setCompositingMode NOTIFY
                   compositingModeChanged)
    Q_PROPERTY(bool useCompositing READ isUseCompositing WRITE setUseCompositing NOTIFY
                   useCompositingChanged)
    Q_PROPERTY(
        int hiddenPreviews READ hiddenPreviews WRITE setHiddenPreviews NOTIFY hiddenPreviewsChanged)
    Q_PROPERTY(qint64 maxFpsInterval READ maxFpsInterval WRITE setMaxFpsInterval NOTIFY
                   maxFpsIntervalChanged)
    Q_PROPERTY(uint refreshRate READ refreshRate WRITE setRefreshRate NOTIFY refreshRateChanged)
    Q_PROPERTY(qint64 vBlankTime READ vBlankTime WRITE setVBlankTime NOTIFY vBlankTimeChanged)
    Q_PROPERTY(bool glStrictBinding READ isGlStrictBinding WRITE setGlStrictBinding NOTIFY
                   glStrictBindingChanged)
    /**
     * Whether strict binding follows the driver or has been overwritten by a user defined config
     * value. If @c true glStrictBinding is set by the OpenGL Scene during initialization. If @c
     * false glStrictBinding is set from a config value and not updated during scene initialization.
     */
    Q_PROPERTY(bool glStrictBindingFollowsDriver READ isGlStrictBindingFollowsDriver WRITE
                   setGlStrictBindingFollowsDriver NOTIFY glStrictBindingFollowsDriverChanged)
    Q_PROPERTY(KWin::OpenGLPlatformInterface glPlatformInterface READ glPlatformInterface WRITE
                   setGlPlatformInterface NOTIFY glPlatformInterfaceChanged)
    Q_PROPERTY(bool windowsBlockCompositing READ windowsBlockCompositing WRITE
                   setWindowsBlockCompositing NOTIFY windowsBlockCompositingChanged)
public:
    /**
     * This enum type is used to specify the focus policy.
     *
     * Note that FocusUnderMouse and FocusStrictlyUnderMouse are not
     * particulary useful. They are only provided for old-fashined
     * die-hard UNIX people ;-)
     */
    enum FocusPolicy {
        /**
         * Clicking into a window activates it. This is also the default.
         */
        ClickToFocus,
        /**
         * Moving the mouse pointer actively onto a normal window activates it.
         * For convenience, the desktop and windows on the dock are excluded.
         * They require clicking.
         */
        FocusFollowsMouse,
        /**
         * The window that happens to be under the mouse pointer becomes active.
         * The invariant is: no window can have focus that is not under the mouse.
         * This also means that Alt-Tab won't work properly and popup dialogs are
         * usually unsable with the keyboard. Note that the desktop and windows on
         * the dock are excluded for convenience. They get focus only when clicking
         * on it.
         */
        FocusUnderMouse,
        /**
         * This is even worse than FocusUnderMouse. Only the window under the mouse
         * pointer is active. If the mouse points nowhere, nothing has the focus. If
         * the mouse points onto the desktop, the desktop has focus. The same holds
         * for windows on the dock.
         */
        FocusStrictlyUnderMouse
    };

    FocusPolicy focusPolicy() const
    {
        return m_focusPolicy;
    }
    bool isNextFocusPrefersMouse() const
    {
        return m_nextFocusPrefersMouse;
    }

    /**
     * Whether clicking on a window raises it in FocusFollowsMouse
     * mode or not.
     */
    bool isClickRaise() const
    {
        return m_clickRaise;
    }

    /**
     * Whether autoraise is enabled FocusFollowsMouse mode or not.
     */
    bool isAutoRaise() const
    {
        return m_autoRaise;
    }

    /**
     * Autoraise interval
     */
    int autoRaiseInterval() const
    {
        return m_autoRaiseInterval;
    }

    /**
     * Delayed focus interval.
     */
    int delayFocusInterval() const
    {
        return m_delayFocusInterval;
    }

    /**
     * Whether to see Xinerama screens separately for focus (in Alt+Tab, when activating next
     * client)
     */
    bool isSeparateScreenFocus() const
    {
        return m_separateScreenFocus;
    }

    win::placement placement() const
    {
        return m_placement;
    }

    bool focusPolicyIsReasonable()
    {
        return m_focusPolicy == ClickToFocus || m_focusPolicy == FocusFollowsMouse;
    }

    /**
     * The size of the zone that triggers snapping on desktop borders.
     */
    int borderSnapZone() const
    {
        return m_borderSnapZone;
    }

    /**
     * The size of the zone that triggers snapping with other windows.
     */
    int windowSnapZone() const
    {
        return m_windowSnapZone;
    }

    /**
     * The size of the zone that triggers snapping on the screen center.
     */
    int centerSnapZone() const
    {
        return m_centerSnapZone;
    }

    /**
     * Snap only when windows will overlap.
     */
    bool isSnapOnlyWhenOverlapping() const
    {
        return m_snapOnlyWhenOverlapping;
    }

    /**
     * Whether or not we roll over to the other edge when switching desktops past the edge.
     */
    bool isRollOverDesktops() const
    {
        return m_rollOverDesktops;
    }

    /**
     * Returns the focus stealing prevention level.
     *
     * @see allowClientActivation
     */
    win::fsp_level focusStealingPreventionLevel() const
    {
        return m_focusStealingPreventionLevel;
    }

    enum WindowOperation {
        MaximizeOp = 5000,
        RestoreOp,
        MinimizeOp,
        MoveOp,
        UnrestrictedMoveOp,
        ResizeOp,
        UnrestrictedResizeOp,
        CloseOp,
        OnAllDesktopsOp,
        KeepAboveOp,
        KeepBelowOp,
        OperationsOp,
        WindowRulesOp,
        ToggleStoreSettingsOp = WindowRulesOp, ///< @obsolete
        HMaximizeOp,
        VMaximizeOp,
        LowerOp,
        FullScreenOp,
        NoBorderOp,
        NoOp,
        SetupWindowShortcutOp,
        ApplicationRulesOp,
    };

    enum AnimationCurve {
        Linear,
        Quadratic,
        Cubic,
        Quartic,
        Sine,
    };

    WindowOperation operationTitlebarDblClick() const
    {
        return OpTitlebarDblClick;
    }
    WindowOperation operationMaxButtonLeftClick() const
    {
        return opMaxButtonLeftClick;
    }
    WindowOperation operationMaxButtonRightClick() const
    {
        return opMaxButtonRightClick;
    }
    WindowOperation operationMaxButtonMiddleClick() const
    {
        return opMaxButtonMiddleClick;
    }
    WindowOperation operationMaxButtonClick(Qt::MouseButtons button) const;

    enum MouseCommand {
        MouseRaise,
        MouseLower,
        MouseOperationsMenu,
        MouseToggleRaiseAndLower,
        MouseActivateAndRaise,
        MouseActivateAndLower,
        MouseActivate,
        MouseActivateRaiseAndPassClick,
        MouseActivateAndPassClick,
        MouseMove,
        MouseUnrestrictedMove,
        MouseActivateRaiseAndMove,
        MouseActivateRaiseAndUnrestrictedMove,
        MouseResize,
        MouseUnrestrictedResize,
        MouseMaximize,
        MouseRestore,
        MouseMinimize,
        MouseNextDesktop,
        MousePreviousDesktop,
        MouseAbove,
        MouseBelow,
        MouseOpacityMore,
        MouseOpacityLess,
        MouseClose,
        MouseNothing
    };

    enum MouseWheelCommand {
        MouseWheelRaiseLower,
        MouseWheelMaximizeRestore,
        MouseWheelAboveBelow,
        MouseWheelPreviousNextDesktop,
        MouseWheelChangeOpacity,
        MouseWheelNothing
    };

    MouseCommand commandActiveTitlebar1() const
    {
        return CmdActiveTitlebar1;
    }
    MouseCommand commandActiveTitlebar2() const
    {
        return CmdActiveTitlebar2;
    }
    MouseCommand commandActiveTitlebar3() const
    {
        return CmdActiveTitlebar3;
    }
    MouseCommand commandInactiveTitlebar1() const
    {
        return CmdInactiveTitlebar1;
    }
    MouseCommand commandInactiveTitlebar2() const
    {
        return CmdInactiveTitlebar2;
    }
    MouseCommand commandInactiveTitlebar3() const
    {
        return CmdInactiveTitlebar3;
    }
    MouseCommand commandWindow1() const
    {
        return CmdWindow1;
    }
    MouseCommand commandWindow2() const
    {
        return CmdWindow2;
    }
    MouseCommand commandWindow3() const
    {
        return CmdWindow3;
    }
    MouseCommand commandWindowWheel() const
    {
        return CmdWindowWheel;
    }
    MouseCommand commandAll1() const
    {
        return CmdAll1;
    }
    MouseCommand commandAll2() const
    {
        return CmdAll2;
    }
    MouseCommand commandAll3() const
    {
        return CmdAll3;
    }
    MouseWheelCommand commandAllWheel() const
    {
        return CmdAllWheel;
    }
    uint keyCmdAllModKey() const
    {
        return CmdAllModKey;
    }
    Qt::KeyboardModifier commandAllModifier() const
    {
        switch (CmdAllModKey) {
        case Qt::Key_Alt:
            return Qt::AltModifier;
        case Qt::Key_Meta:
            return Qt::MetaModifier;
        default:
            Q_UNREACHABLE();
        }
    }

    /**
     * Returns whether the user prefers his caption clean.
     */
    bool condensedTitle() const;

    /**
     * @returns true if a window gets maximized when it reaches top screen edge
     * while being moved.
     */
    bool electricBorderMaximize() const
    {
        return electric_border_maximize;
    }
    /**
     * @returns true if window is tiled to half screen when reaching left or
     * right screen edge while been moved.
     */
    bool electricBorderTiling() const
    {
        return electric_border_tiling;
    }
    /**
     * @returns the factor that determines the corner part of the edge (ie. 0.1 means tiny corner)
     */
    float electricBorderCornerRatio() const
    {
        return electric_border_corner_ratio;
    }

    bool borderlessMaximizedWindows() const
    {
        return borderless_maximized_windows;
    }

    /**
     * Timeout before non-responding application will be killed after attempt to close.
     */
    int killPingTimeout() const
    {
        return m_killPingTimeout;
    }

    /**
     * Whether to hide utility windows for inactive applications.
     */
    bool isHideUtilityWindowsForInactive() const
    {
        return m_hideUtilityWindowsForInactive;
    }

    //----------------------
    // Compositing settings
    CompositingType compositingMode() const
    {
        return m_compositingMode;
    }
    void setCompositingMode(CompositingType mode)
    {
        m_compositingMode = mode;
    }
    // Separate to mode so the user can toggle
    bool isUseCompositing() const;

    // General preferences
    HiddenPreviews hiddenPreviews() const
    {
        return m_hiddenPreviews;
    }

    qint64 maxFpsInterval() const
    {
        return m_maxFpsInterval;
    }
    // Settings that should be auto-detected
    uint refreshRate() const
    {
        return m_refreshRate;
    }
    qint64 vBlankTime() const
    {
        return m_vBlankTime;
    }
    bool isGlStrictBinding() const
    {
        return m_glStrictBinding;
    }
    bool isGlStrictBindingFollowsDriver() const
    {
        return m_glStrictBindingFollowsDriver;
    }
    OpenGLPlatformInterface glPlatformInterface() const
    {
        return m_glPlatformInterface;
    }

    bool windowsBlockCompositing() const
    {
        return m_windowsBlockCompositing;
    }

    AnimationCurve animationCurve() const
    {
        return m_animationCurve;
    }

    // setters
    void setFocusPolicy(FocusPolicy focusPolicy);
    void setNextFocusPrefersMouse(bool nextFocusPrefersMouse);
    void setClickRaise(bool clickRaise);
    void setAutoRaise(bool autoRaise);
    void setAutoRaiseInterval(int autoRaiseInterval);
    void setDelayFocusInterval(int delayFocusInterval);
    void setSeparateScreenFocus(bool separateScreenFocus);
    void setPlacement(win::placement placement);
    void setBorderSnapZone(int borderSnapZone);
    void setWindowSnapZone(int windowSnapZone);
    void setCenterSnapZone(int centerSnapZone);
    void setSnapOnlyWhenOverlapping(bool snapOnlyWhenOverlapping);
    void setRollOverDesktops(bool rollOverDesktops);
    void setFocusStealingPreventionLevel(win::fsp_level focusStealingPreventionLevel);
    void setOperationTitlebarDblClick(WindowOperation operationTitlebarDblClick);
    void setOperationMaxButtonLeftClick(WindowOperation op);
    void setOperationMaxButtonRightClick(WindowOperation op);
    void setOperationMaxButtonMiddleClick(WindowOperation op);
    void setCommandActiveTitlebar1(MouseCommand commandActiveTitlebar1);
    void setCommandActiveTitlebar2(MouseCommand commandActiveTitlebar2);
    void setCommandActiveTitlebar3(MouseCommand commandActiveTitlebar3);
    void setCommandInactiveTitlebar1(MouseCommand commandInactiveTitlebar1);
    void setCommandInactiveTitlebar2(MouseCommand commandInactiveTitlebar2);
    void setCommandInactiveTitlebar3(MouseCommand commandInactiveTitlebar3);
    void setCommandWindow1(MouseCommand commandWindow1);
    void setCommandWindow2(MouseCommand commandWindow2);
    void setCommandWindow3(MouseCommand commandWindow3);
    void setCommandWindowWheel(MouseCommand commandWindowWheel);
    void setCommandAll1(MouseCommand commandAll1);
    void setCommandAll2(MouseCommand commandAll2);
    void setCommandAll3(MouseCommand commandAll3);
    void setKeyCmdAllModKey(uint keyCmdAllModKey);
    void setCondensedTitle(bool condensedTitle);
    void setElectricBorderMaximize(bool electricBorderMaximize);
    void setElectricBorderTiling(bool electricBorderTiling);
    void setElectricBorderCornerRatio(float electricBorderCornerRatio);
    void setBorderlessMaximizedWindows(bool borderlessMaximizedWindows);
    void setKillPingTimeout(int killPingTimeout);
    void setHideUtilityWindowsForInactive(bool hideUtilityWindowsForInactive);
    void setCompositingMode(int compositingMode);
    void setUseCompositing(bool useCompositing);
    void setHiddenPreviews(int hiddenPreviews);
    void setMaxFpsInterval(qint64 maxFpsInterval);
    void setRefreshRate(uint refreshRate);
    void setVBlankTime(qint64 vBlankTime);
    void setGlStrictBinding(bool glStrictBinding);
    void setGlStrictBindingFollowsDriver(bool glStrictBindingFollowsDriver);
    void setGlPlatformInterface(OpenGLPlatformInterface interface);
    void setWindowsBlockCompositing(bool set);
    void setAnimationCurve(AnimationCurve curve);

    // default values
    static WindowOperation defaultOperationTitlebarDblClick()
    {
        return MaximizeOp;
    }
    static WindowOperation defaultOperationMaxButtonLeftClick()
    {
        return MaximizeOp;
    }
    static WindowOperation defaultOperationMaxButtonRightClick()
    {
        return HMaximizeOp;
    }
    static WindowOperation defaultOperationMaxButtonMiddleClick()
    {
        return VMaximizeOp;
    }
    static MouseCommand defaultCommandActiveTitlebar1()
    {
        return MouseRaise;
    }
    static MouseCommand defaultCommandActiveTitlebar2()
    {
        return MouseNothing;
    }
    static MouseCommand defaultCommandActiveTitlebar3()
    {
        return MouseOperationsMenu;
    }
    static MouseCommand defaultCommandInactiveTitlebar1()
    {
        return MouseActivateAndRaise;
    }
    static MouseCommand defaultCommandInactiveTitlebar2()
    {
        return MouseNothing;
    }
    static MouseCommand defaultCommandInactiveTitlebar3()
    {
        return MouseOperationsMenu;
    }
    static MouseCommand defaultCommandWindow1()
    {
        return MouseActivateRaiseAndPassClick;
    }
    static MouseCommand defaultCommandWindow2()
    {
        return MouseActivateAndPassClick;
    }
    static MouseCommand defaultCommandWindow3()
    {
        return MouseActivateAndPassClick;
    }
    static MouseCommand defaultCommandWindowWheel()
    {
        return MouseNothing;
    }
    static MouseCommand defaultCommandAll1()
    {
        return MouseUnrestrictedMove;
    }
    static MouseCommand defaultCommandAll2()
    {
        return MouseToggleRaiseAndLower;
    }
    static MouseCommand defaultCommandAll3()
    {
        return MouseUnrestrictedResize;
    }
    static MouseWheelCommand defaultCommandTitlebarWheel()
    {
        return MouseWheelNothing;
    }
    static MouseWheelCommand defaultCommandAllWheel()
    {
        return MouseWheelNothing;
    }
    static uint defaultKeyCmdAllModKey()
    {
        return Qt::Key_Alt;
    }
    static CompositingType defaultCompositingMode()
    {
        return OpenGLCompositing;
    }
    static bool defaultUseCompositing()
    {
        return true;
    }
    static HiddenPreviews defaultHiddenPreviews()
    {
        return HiddenPreviewsShown;
    }
    static qint64 defaultMaxFpsInterval()
    {
        return (1 * 1000 * 1000 * 1000) / 60.0; // nanoseconds / Hz
    }
    static int defaultMaxFps()
    {
        return 60;
    }
    static uint defaultRefreshRate()
    {
        return 0;
    }
    static uint defaultVBlankTime()
    {
        return 6000; // 6ms
    }
    static bool defaultGlStrictBinding()
    {
        return true;
    }
    static bool defaultGlStrictBindingFollowsDriver()
    {
        return true;
    }

    //----------------------
Q_SIGNALS:
    // for properties
    void focusPolicyChanged();
    void focusPolicyIsResonableChanged();
    void nextFocusPrefersMouseChanged();
    void clickRaiseChanged();
    void autoRaiseChanged();
    void autoRaiseIntervalChanged();
    void delayFocusIntervalChanged();
    void separateScreenFocusChanged(bool);
    void placementChanged();
    void borderSnapZoneChanged();
    void windowSnapZoneChanged();
    void centerSnapZoneChanged();
    void snapOnlyWhenOverlappingChanged();
    void rollOverDesktopsChanged(bool enabled);
    void focusStealingPreventionLevelChanged();
    void operationTitlebarDblClickChanged();
    void operationMaxButtonLeftClickChanged();
    void operationMaxButtonRightClickChanged();
    void operationMaxButtonMiddleClickChanged();
    void commandActiveTitlebar1Changed();
    void commandActiveTitlebar2Changed();
    void commandActiveTitlebar3Changed();
    void commandInactiveTitlebar1Changed();
    void commandInactiveTitlebar2Changed();
    void commandInactiveTitlebar3Changed();
    void commandWindow1Changed();
    void commandWindow2Changed();
    void commandWindow3Changed();
    void commandWindowWheelChanged();
    void commandAll1Changed();
    void commandAll2Changed();
    void commandAll3Changed();
    void keyCmdAllModKeyChanged();
    void condensedTitleChanged();
    void electricBorderMaximizeChanged();
    void electricBorderTilingChanged();
    void electricBorderCornerRatioChanged();
    void borderlessMaximizedWindowsChanged();
    void killPingTimeoutChanged();
    void hideUtilityWindowsForInactiveChanged();
    void compositingModeChanged();
    void useCompositingChanged();
    void hiddenPreviewsChanged();
    void maxFpsIntervalChanged();
    void refreshRateChanged();
    void vBlankTimeChanged();
    void glStrictBindingChanged();
    void glStrictBindingFollowsDriverChanged();
    void glPlatformInterfaceChanged();
    void windowsBlockCompositingChanged();
    void animationSpeedChanged();
    void animationCurveChanged();

    void configChanged();

private:
    FocusPolicy m_focusPolicy{ClickToFocus};
    bool m_nextFocusPrefersMouse{false};
    bool m_clickRaise{false};
    bool m_autoRaise{false};
    int m_autoRaiseInterval{0};
    int m_delayFocusInterval{0};

    bool m_separateScreenFocus{false};

    win::placement m_placement{win::placement::no_placement};
    int m_borderSnapZone{0};
    int m_windowSnapZone{0};
    int m_centerSnapZone{0};
    bool m_snapOnlyWhenOverlapping{false};
    bool m_rollOverDesktops{false};
    win::fsp_level m_focusStealingPreventionLevel{win::fsp_level::none};
    int m_killPingTimeout{0};
    bool m_hideUtilityWindowsForInactive{false};

    CompositingType m_compositingMode{defaultCompositingMode()};
    bool m_useCompositing{defaultUseCompositing()};
    HiddenPreviews m_hiddenPreviews{defaultHiddenPreviews()};
    qint64 m_maxFpsInterval{defaultMaxFpsInterval()};
    // Settings that should be auto-detected
    uint m_refreshRate{defaultRefreshRate()};
    qint64 m_vBlankTime{defaultVBlankTime()};
    bool m_glStrictBinding{defaultGlStrictBinding()};
    bool m_glStrictBindingFollowsDriver{defaultGlStrictBindingFollowsDriver()};
    OpenGLPlatformInterface m_glPlatformInterface{defaultGlPlatformInterface()};
    bool m_windowsBlockCompositing{true};
    AnimationCurve m_animationCurve{AnimationCurve::Linear};

    WindowOperation OpTitlebarDblClick{defaultOperationTitlebarDblClick()};
    WindowOperation opMaxButtonRightClick{defaultOperationMaxButtonRightClick()};
    WindowOperation opMaxButtonMiddleClick{defaultOperationMaxButtonMiddleClick()};
    WindowOperation opMaxButtonLeftClick{defaultOperationMaxButtonRightClick()};

    // mouse bindings
    MouseCommand CmdActiveTitlebar1{defaultCommandActiveTitlebar1()};
    MouseCommand CmdActiveTitlebar2{defaultCommandActiveTitlebar2()};
    MouseCommand CmdActiveTitlebar3{defaultCommandActiveTitlebar3()};
    MouseCommand CmdInactiveTitlebar1{defaultCommandInactiveTitlebar1()};
    MouseCommand CmdInactiveTitlebar2{defaultCommandInactiveTitlebar2()};
    MouseCommand CmdInactiveTitlebar3{defaultCommandInactiveTitlebar3()};
    MouseWheelCommand CmdTitlebarWheel{defaultCommandTitlebarWheel()};
    MouseCommand CmdWindow1{defaultCommandWindow1()};
    MouseCommand CmdWindow2{defaultCommandWindow2()};
    MouseCommand CmdWindow3{defaultCommandWindow3()};
    MouseCommand CmdWindowWheel{defaultCommandWindowWheel()};
    MouseCommand CmdAll1{defaultCommandAll1()};
    MouseCommand CmdAll2{defaultCommandAll2()};
    MouseCommand CmdAll3{defaultCommandAll3()};
    MouseWheelCommand CmdAllWheel{defaultCommandAllWheel()};
    uint CmdAllModKey{defaultKeyCmdAllModKey()};

    bool electric_border_maximize{false};
    bool electric_border_tiling{false};
    float electric_border_corner_ratio{0.};
    bool borderless_maximized_windows{false};
    bool condensed_title{false};

    friend class options;
};

class KWIN_EXPORT options
{
public:
    options();
    ~options();

    void updateSettings();

    void reloadCompositingSettings(bool force = false);

    /**
     * Performs loading all settings except compositing related.
     */
    void loadConfig();
    /**
     * Performs loading of compositing settings which do not depend on OpenGL.
     */
    bool loadCompositingConfig(bool force);

    /**
     * Returns the animation time factor for desktop effects.
     */
    double animationTimeFactor() const;

    bool get_current_output_follows_mouse() const;
    QStringList modifierOnlyDBusShortcut(Qt::KeyboardModifier mod) const;

    static options_qobject::WindowOperation windowOperation(const QString& name, bool restricted);
    static options_qobject::MouseCommand mouseCommand(const QString& name, bool restricted);
    static options_qobject::MouseWheelCommand mouseWheelCommand(const QString& name);

    options_qobject::MouseCommand operationTitlebarMouseWheel(int delta) const
    {
        return wheelToMouseCommand(qobject->CmdTitlebarWheel, delta);
    }
    options_qobject::MouseCommand operationWindowMouseWheel(int delta) const
    {
        return wheelToMouseCommand(qobject->CmdAllWheel, delta);
    }

    std::unique_ptr<options_qobject> qobject;

private:
    void syncFromKcfgc();

    options_qobject::MouseCommand wheelToMouseCommand(options_qobject::MouseWheelCommand com,
                                                      int delta) const;

    QScopedPointer<Settings> m_settings;
    KConfigWatcher::Ptr m_configWatcher;

    bool current_output_follows_mouse{false};
    QHash<Qt::KeyboardModifier, QStringList> m_modifierOnlyShortcuts;
};

}

Q_DECLARE_METATYPE(KWin::base::options_qobject::WindowOperation)
Q_DECLARE_METATYPE(KWin::OpenGLPlatformInterface)
Q_DECLARE_METATYPE(KWin::win::fsp_level)
Q_DECLARE_METATYPE(KWin::win::placement)

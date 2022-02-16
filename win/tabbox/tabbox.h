/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>
Copyright (C) 2009 Martin Gräßlin <mgraesslin@kde.org>

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
#ifndef KWIN_TABBOX_H
#define KWIN_TABBOX_H

#include "tabbox_handler.h"

#include "kwin_export.h"
#include "kwinglobals.h"

#include <memory>

#include <QKeySequence>
#include <QModelIndex>
#include <QTimer>

class KConfigGroup;
class QAction;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

namespace KWin
{
class Toplevel;

namespace base::x11
{
class event_filter;
}

namespace TabBox
{
class DesktopChainManager;
class TabBoxConfig;
class TabBox;
class TabBoxHandlerImpl : public TabBoxHandler
{
public:
    explicit TabBoxHandlerImpl(TabBox* tabBox);
    ~TabBoxHandlerImpl() override;

    int active_screen() const override;
    std::weak_ptr<TabBoxClient> active_client() const override;
    int current_desktop() const override;
    QString desktop_name(TabBoxClient* client) const override;
    QString desktop_name(int desktop) const override;
    bool is_kwin_compositing() const override;
    std::weak_ptr<TabBoxClient> next_client_focus_chain(TabBoxClient* client) const override;
    std::weak_ptr<TabBoxClient> first_client_focus_chain() const override;
    bool is_in_focus_chain(TabBoxClient* client) const override;
    int next_desktop_focus_chain(int desktop) const override;
    int number_of_desktops() const override;
    TabBoxClientList stacking_order() const override;
    void elevate_client(TabBoxClient* c, QWindow* tabbox, bool elevate) const override;
    void raise_client(TabBoxClient* client) const override;
    void restack(TabBoxClient* c, TabBoxClient* under) override;
    std::weak_ptr<TabBoxClient> client_to_add_to_list(KWin::TabBox::TabBoxClient* client,
                                                      int desktop) const override;
    std::weak_ptr<TabBoxClient> desktop_client() const override;
    void activate_and_close() override;
    void highlight_windows(TabBoxClient* window = nullptr, QWindow* controller = nullptr) override;
    bool no_modifier_grab() const override;

private:
    bool check_desktop(TabBoxClient* client, int desktop) const;
    bool check_applications(TabBoxClient* client) const;
    bool check_minimized(TabBoxClient* client) const;
    bool check_multi_screen(TabBoxClient* client) const;

    TabBox* m_tabbox;
    DesktopChainManager* m_desktop_focus_chain;
};

class TabBoxClientImpl : public TabBoxClient
{
public:
    explicit TabBoxClientImpl(Toplevel* window);
    ~TabBoxClientImpl() override;

    QString caption() const override;
    QIcon icon() const override;
    bool is_minimized() const override;
    int x() const override;
    int y() const override;
    int width() const override;
    int height() const override;
    bool is_closeable() const override;
    void close() override;
    bool is_first_in_tabbox() const override;
    QUuid internal_id() const override;

    Toplevel* client() const
    {
        return m_client;
    }

private:
    Toplevel* m_client;
};

class KWIN_EXPORT TabBox : public QObject
{
    Q_OBJECT
public:
    ~TabBox() override;

    /**
     * Returns the currently displayed client ( only works in TabBoxWindowsMode ).
     * Returns 0 if no client is displayed.
     */
    Toplevel* current_client();

    /**
     * Returns the list of clients potentially displayed ( only works in
     * TabBoxWindowsMode ).
     * Returns an empty list if no clients are available.
     */
    QList<Toplevel*> current_client_list();

    /**
     * Returns the currently displayed virtual desktop ( only works in
     * TabBoxDesktopListMode )
     * Returns -1 if no desktop is displayed.
     */
    int current_desktop();

    /**
     * Returns the list of desktops potentially displayed ( only works in
     * TabBoxDesktopListMode )
     * Returns an empty list if no are available.
     */
    QList<int> current_desktop_list();

    /**
     * Change the currently selected client, and notify the effects.
     *
     * @see setCurrentDesktop
     */
    void set_current_client(Toplevel* window);

    /**
     * Change the currently selected desktop, and notify the effects.
     *
     * @see setCurrentClient
     */
    void set_current_desktop(int new_desktop);

    /**
     * Sets the current mode to \a mode, either TabBoxDesktopListMode or TabBoxWindowsMode
     *
     * @see mode
     */
    void set_mode(TabBoxMode mode);
    TabBoxMode mode() const
    {
        return m_tabbox_mode;
    }

    /**
     * Resets the tab box to display the active client in TabBoxWindowsMode, or the
     * current desktop in TabBoxDesktopListMode
     */
    void reset(bool partial_reset = false);

    /**
     * Shows the next or previous item, depending on \a next
     */
    void next_prev(bool next = true);

    /**
     * Shows the tab box after some delay.
     *
     * If the 'ShowDelay' setting is false, show() is simply called.
     *
     * Otherwise, we start a timer for the delay given in the settings and only
     * do a show() when it times out.
     *
     * This means that you can alt-tab between windows and you don't see the
     * tab box immediately. Not only does this make alt-tabbing faster, it gives
     * less 'flicker' to the eyes. You don't need to see the tab box if you're
     * just quickly switching between 2 or 3 windows. It seems to work quite
     * nicely.
     */
    void delayed_show();

    /**
     * Notify effects that the tab box is being hidden.
     */
    void hide(bool abort = false);

    /**
     * Increases the reference count, preventing the default tabbox from showing.
     *
     * @see unreference
     * @see isDisplayed
     */
    void reference()
    {
        ++m_display_ref_count;
    }

    /**
     * Decreases the reference count. Only when the reference count is 0 will
     * the default tab box be shown.
     */
    void unreference()
    {
        --m_display_ref_count;
    }

    /**
     * Returns whether the tab box is being displayed, either natively or by an
     * effect.
     *
     * @see reference
     * @see unreference
     */
    bool is_displayed() const
    {
        return m_display_ref_count > 0;
    }

    /**
     * @returns @c true if TabBox is shown, @c false if replaced by Effect
     */
    bool is_shown() const
    {
        return m_is_shown;
    }

    bool handle_mouse_event(QMouseEvent* event);
    bool handle_wheel_event(QWheelEvent* event);
    void grabbed_key_event(QKeyEvent* event);

    bool is_grabbed() const
    {
        return m_tab_grab || m_desktop_grab;
    }

    void init_shortcuts();

    Toplevel* next_client_static(Toplevel*) const;
    Toplevel* previous_client_static(Toplevel*) const;
    int next_desktop_static(int iDesktop) const;
    int previous_desktop_static(int iDesktop) const;
    void key_press(int key);
    void modifiers_released();

    bool forced_global_mouse_grab() const
    {
        return m_forced_global_mouse_grab;
    }

    bool no_modifier_grab() const
    {
        return m_no_modifier_grab;
    }
    void set_current_index(QModelIndex index, bool notify_effects = true);

    static TabBox* self();
    static TabBox* create(QObject* parent);

public Q_SLOTS:
    /**
     * Notify effects that the tab box is being shown, and only display the
     * default tab box QFrame if no effect has referenced the tab box.
     */
    void show();
    void close(bool abort = false);
    void accept(bool close_tabbox = true);
    void slot_walk_through_desktops();
    void slot_walk_back_through_desktops();
    void slot_walk_through_desktop_list();
    void slot_walk_back_through_desktop_list();
    void slot_walk_through_windows();
    void slot_walk_back_through_windows();
    void slot_walk_through_windows_alternative();
    void slot_walk_back_through_windows_alternative();
    void slot_walk_through_current_app_windows();
    void slot_walk_back_through_current_app_windows();
    void slot_walk_through_current_app_windows_alternative();
    void slot_walk_back_through_current_app_windows_alternative();

    void handler_ready();

    bool toggle(ElectricBorder eb);

Q_SIGNALS:
    void tabbox_added(int);
    void tabbox_closed();
    void tabbox_updated();
    void tabbox_key_event(QKeyEvent*);

private:
    explicit TabBox(QObject* parent);
    void load_config(const KConfigGroup& config, TabBoxConfig& tabbox_config);

    bool start_kde_walk_through_windows(
        TabBoxMode mode); // TabBoxWindowsMode | TabBoxWindowsAlternativeMode
    bool start_walk_through_desktops(TabBoxMode mode); // TabBoxDesktopMode | TabBoxDesktopListMode
    bool start_walk_through_desktops();
    bool start_walk_through_desktop_list();
    void
    navigating_through_windows(bool forward,
                               const QKeySequence& shortcut,
                               TabBoxMode mode); // TabBoxWindowsMode | TabBoxWindowsAlternativeMode
    void kde_walk_through_windows(bool forward);
    void cde_walk_through_windows(bool forward);
    void walk_through_desktops(bool forward);
    void kde_one_step_through_windows(
        bool forward,
        TabBoxMode mode); // TabBoxWindowsMode | TabBoxWindowsAlternativeMode
    void one_step_through_desktops(bool forward,
                                   TabBoxMode mode); // TabBoxDesktopMode | TabBoxDesktopListMode
    void one_step_through_desktops(bool forward);
    void one_step_through_desktop_list(bool forward);
    bool establish_tabbox_grab();
    void remove_tabbox_grab();
    template<typename Slot>
    void key(const char* action_name, Slot slot, const QKeySequence& shortcut = QKeySequence());

    bool toggle_mode(TabBoxMode mode);

private Q_SLOTS:
    void reconfigure();
    void global_shortcut_changed(QAction* action, const QKeySequence& seq);

private:
    TabBoxMode m_tabbox_mode;
    TabBoxHandlerImpl* m_tabbox;
    bool m_delay_show;
    int m_delay_show_time;

    QTimer m_delayed_show_timer;
    int m_display_ref_count;

    TabBoxConfig m_default_config;
    TabBoxConfig m_alternative_config;
    TabBoxConfig m_default_current_application_config;
    TabBoxConfig m_alternative_current_application_config;
    TabBoxConfig m_desktop_config;
    TabBoxConfig m_desktop_list_config;
    // false if an effect has referenced the tabbox
    // true if tabbox is active (independent on showTabbox setting)
    bool m_is_shown;
    bool m_desktop_grab;
    bool m_tab_grab;
    // true if tabbox is in modal mode which does not require holding a modifier
    bool m_no_modifier_grab;
    QKeySequence m_cut_walk_through_desktops, m_cut_walk_through_desktops_reverse;
    QKeySequence m_cut_walk_through_desktop_list, m_cut_walk_through_desktop_list_reverse;
    QKeySequence m_cut_walk_through_windows, m_cut_walk_through_windows_reverse;
    QKeySequence m_cut_walk_through_windows_alternative,
        m_cut_walk_through_windows_alternative_reverse;
    QKeySequence m_cut_walk_through_current_app_windows,
        m_cut_walk_through_current_app_windows_reverse;
    QKeySequence m_cut_walk_through_current_app_windows_alternative,
        m_cut_walk_through_current_app_windows_alternative_reverse;
    bool m_forced_global_mouse_grab;
    bool m_ready; // indicates whether the config is completely loaded
    QList<ElectricBorder> m_border_activate, m_border_alternative_activate;
    QHash<ElectricBorder, QAction*> m_touch_activate;
    QHash<ElectricBorder, QAction*> m_touch_alternative_activate;
    QScopedPointer<base::x11::event_filter> m_x11_event_filter;

    static TabBox* s_self;
};

inline TabBox* TabBox::self()
{
    return s_self;
}

} // namespace TabBox
} // namespace
#endif

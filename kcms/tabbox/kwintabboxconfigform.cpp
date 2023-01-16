/*
SPDX-FileCopyrightText: 2009 Martin Gräßlin <mgraesslin@kde.org>
SPDX-FileCopyrightText: 2020 Cyril Rossi <cyril.rossi@enioka.com>
SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>

SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kwintabboxconfigform.h"
#include "kwintabboxsettings.h"
#include "ui_main.h"

#include <KActionCollection>
#include <KGlobalAccel>

namespace KWin
{

using namespace win;

KWinTabBoxConfigForm::KWinTabBoxConfigForm(TabboxType type, TabBoxSettings *config, QWidget *parent)
    : QWidget(parent)
    , m_config(config)
    , ui(new Ui::KWinTabBoxConfigForm)
{
    ui->setupUi(this);

    ui->effectConfigButton->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));

    if (QApplication::screens().count() < 2) {
        ui->filterScreens->hide();
        ui->screenFilter->hide();
    }

    connect(this, &KWinTabBoxConfigForm::configChanged, this, &KWinTabBoxConfigForm::updateDefaultIndicators);

    connect(ui->effectConfigButton, &QPushButton::clicked, this, &KWinTabBoxConfigForm::effectConfigButtonClicked);
    connect(ui->kcfg_ShowTabBox, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::tabBoxToggled);

    connect(ui->filterScreens, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterScreen);
    connect(ui->currentScreen, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterScreen);
    connect(ui->otherScreens, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterScreen);

    connect(ui->filterDesktops, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterDesktop);
    connect(ui->currentDesktop, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterDesktop);
    connect(ui->otherDesktops, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterDesktop);

    connect(ui->filterMinimization, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterMinimization);
    connect(ui->visibleWindows, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterMinimization);
    connect(ui->hiddenWindows, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onFilterMinimization);

    connect(ui->oneAppWindow, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onApplicationMode);
    connect(ui->showDesktop, &QAbstractButton::clicked, this, &KWinTabBoxConfigForm::onShowDesktopMode);

    connect(ui->switchingModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &KWinTabBoxConfigForm::onSwitchingMode);
    connect(ui->effectCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &KWinTabBoxConfigForm::onEffectCombo);

    auto addShortcut = [this](const char *name, KKeySequenceWidget *widget, const QKeySequence &sequence = QKeySequence()) {
        QAction *action = m_actionCollection->addAction(name);
        action->setProperty("isConfigurationAction", true);
        action->setText(i18n(name));

        m_actionCollection->setDefaultShortcut(action, sequence);

        widget->setCheckActionCollections({m_actionCollection});
        widget->setProperty("shortcutAction", name);
        connect(widget, &KKeySequenceWidget::keySequenceChanged, this, &KWinTabBoxConfigForm::onShortcutChanged);
    };

    // Shortcut config. The shortcut belongs to the component "kwin"!
    m_actionCollection = new KActionCollection(this, QStringLiteral("kwin"));
    m_actionCollection->setComponentDisplayName(i18n("KWin"));
    m_actionCollection->setConfigGroup("Navigation");
    m_actionCollection->setConfigGlobal(true);

    if (TabboxType::Main == type) {
        addShortcut("Walk Through Windows", ui->scAll, static_cast<Qt::Key>(Qt::ALT) + Qt::Key_Tab);
        addShortcut("Walk Through Windows (Reverse)",
                    ui->scAllReverse,
                    static_cast<Qt::Key>(Qt::ALT + Qt::SHIFT) + Qt::Key_Backtab);
        addShortcut("Walk Through Windows of Current Application",
                    ui->scCurrent,
                    static_cast<Qt::Key>(Qt::ALT) + Qt::Key_QuoteLeft);
        addShortcut("Walk Through Windows of Current Application (Reverse)",
                    ui->scCurrentReverse,
                    static_cast<Qt::Key>(Qt::ALT) + Qt::Key_AsciiTilde);
    } else if (TabboxType::Alternative == type) {
        addShortcut("Walk Through Windows Alternative", ui->scAll);
        addShortcut("Walk Through Windows Alternative (Reverse)", ui->scAllReverse);
        addShortcut("Walk Through Windows of Current Application Alternative", ui->scCurrent);
        addShortcut("Walk Through Windows of Current Application Alternative (Reverse)", ui->scCurrentReverse);
    }

    updateUiFromConfig();
}

KWinTabBoxConfigForm::~KWinTabBoxConfigForm()
{
    delete ui;
}

TabBoxSettings *KWinTabBoxConfigForm::config() const
{
    return m_config;
}

bool KWinTabBoxConfigForm::highlightWindows() const
{
    return ui->kcfg_HighlightWindows->isChecked();
}

bool KWinTabBoxConfigForm::showTabBox() const
{
    return ui->kcfg_ShowTabBox->isChecked();
}

int KWinTabBoxConfigForm::filterScreen() const
{
    if (ui->filterScreens->isChecked()) {
        return ui->currentScreen->isChecked() ? tabbox_config::OnlyCurrentScreenClients : tabbox_config::ExcludeCurrentScreenClients;
    } else {
        return tabbox_config::IgnoreMultiScreen;
    }
}

int KWinTabBoxConfigForm::filterDesktop() const
{
    if (ui->filterDesktops->isChecked()) {
        return ui->currentDesktop->isChecked() ? tabbox_config::OnlyCurrentDesktopClients : tabbox_config::ExcludeCurrentDesktopClients;
    } else {
        return tabbox_config::AllDesktopsClients;
    }
}

int KWinTabBoxConfigForm::filterMinimization() const
{
    if (ui->filterMinimization->isChecked()) {
        return ui->visibleWindows->isChecked() ? tabbox_config::ExcludeMinimizedClients : tabbox_config::OnlyMinimizedClients;
    } else {
        return tabbox_config::IgnoreMinimizedStatus;
    }
}

int KWinTabBoxConfigForm::applicationMode() const
{
    return ui->oneAppWindow->isChecked() ? tabbox_config::OneWindowPerApplication : tabbox_config::AllWindowsAllApplications;
}

int KWinTabBoxConfigForm::showDesktopMode() const
{
    return ui->showDesktop->isChecked() ? tabbox_config::ShowDesktopClient : tabbox_config::DoNotShowDesktopClient;
}

int KWinTabBoxConfigForm::switchingMode() const
{
    return ui->switchingModeCombo->currentIndex();
}

QString KWinTabBoxConfigForm::layoutName() const
{
    return ui->effectCombo->currentData().toString();
}

void KWinTabBoxConfigForm::setFilterScreen(win::tabbox_config::ClientMultiScreenMode mode)
{
    ui->filterScreens->setChecked(mode != tabbox_config::IgnoreMultiScreen);
    ui->currentScreen->setChecked(mode == tabbox_config::OnlyCurrentScreenClients);
    ui->otherScreens->setChecked(mode == tabbox_config::ExcludeCurrentScreenClients);
}

void KWinTabBoxConfigForm::setFilterDesktop(win::tabbox_config::ClientDesktopMode mode)
{
    ui->filterDesktops->setChecked(mode != tabbox_config::AllDesktopsClients);
    ui->currentDesktop->setChecked(mode == tabbox_config::OnlyCurrentDesktopClients);
    ui->otherDesktops->setChecked(mode == tabbox_config::ExcludeCurrentDesktopClients);
}

void KWinTabBoxConfigForm::setFilterMinimization(win::tabbox_config::ClientMinimizedMode mode)
{
    ui->filterMinimization->setChecked(mode != tabbox_config::IgnoreMinimizedStatus);
    ui->visibleWindows->setChecked(mode == tabbox_config::ExcludeMinimizedClients);
    ui->hiddenWindows->setChecked(mode == tabbox_config::OnlyMinimizedClients);
}

void KWinTabBoxConfigForm::setApplicationMode(win::tabbox_config::ClientApplicationsMode mode)
{
    ui->oneAppWindow->setChecked(mode == tabbox_config::OneWindowPerApplication);
}

void KWinTabBoxConfigForm::setShowDesktopMode(win::tabbox_config::ShowDesktopMode mode)
{
    ui->showDesktop->setChecked(mode == tabbox_config::ShowDesktopClient);
}

void KWinTabBoxConfigForm::setSwitchingModeChanged(win::tabbox_config::ClientSwitchingMode mode)
{
    ui->switchingModeCombo->setCurrentIndex(mode);
}

void KWinTabBoxConfigForm::setLayoutName(const QString &layoutName)
{
    const int index = ui->effectCombo->findData(layoutName);
    if (index >= 0) {
        ui->effectCombo->setCurrentIndex(index);
    }
}

void KWinTabBoxConfigForm::setEffectComboModel(QStandardItemModel *model)
{
    // We don't want to lose the config layout when resetting the combo model
    auto const layout = m_config->layoutName();
    ui->effectCombo->setModel(model);
    setLayoutName(layout);
}

QVariant KWinTabBoxConfigForm::effectComboCurrentData(int role) const
{
    return ui->effectCombo->currentData(role);
}

void KWinTabBoxConfigForm::loadShortcuts()
{
    for (const auto &widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        const auto shortcuts = KGlobalAccel::self()->globalShortcut(QStringLiteral("kwin"), actionName);
        if (!shortcuts.isEmpty()) {
            widget->setKeySequence(shortcuts.first());
        }
    }
}

void KWinTabBoxConfigForm::resetShortcuts()
{
    for (auto const& widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        QAction *action = m_actionCollection->action(actionName);
        widget->setKeySequence(m_actionCollection->defaultShortcut(action));
    }
}

void KWinTabBoxConfigForm::saveShortcuts()
{
    for (const auto &widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        QAction *action = m_actionCollection->action(actionName);
        KGlobalAccel::self()->setShortcut(action, {action->shortcut()}, KGlobalAccel::NoAutoloading);
    }
}

bool KWinTabBoxConfigForm::isShortcutsChanged() const
{
    for (const auto &widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        QAction *action = m_actionCollection->action(actionName);
        const auto shortcuts = KGlobalAccel::self()->globalShortcut(QStringLiteral("kwin"), actionName);
        const QKeySequence savedShortcut = shortcuts.isEmpty() ? QKeySequence() : shortcuts.first();

        if (action->shortcut() != savedShortcut) {
            return true;
        }
    }
    return false;
}

bool KWinTabBoxConfigForm::isShortcutsDefault() const
{
    for (const auto &widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        QAction *action = m_actionCollection->action(actionName);
        if (action->shortcut() != m_actionCollection->defaultShortcut(action)) {
            return false;
        }
    }
    return true;
}

void KWinTabBoxConfigForm::tabBoxToggled(bool on)
{
    // Highlight Windows options is availabled if no TabBox effect is selected
    // or if Tabbox is not builtin effet.
    on = !on || ui->effectCombo->currentData(AddonEffect).toBool();
    ui->kcfg_HighlightWindows->setEnabled(on && m_isHighlightWindowsEnabled);
}

void KWinTabBoxConfigForm::onFilterScreen()
{
    m_config->setMultiScreenMode(filterScreen());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onFilterDesktop()
{
    m_config->setDesktopMode(filterDesktop());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onFilterMinimization()
{
    m_config->setMinimizedMode(filterMinimization());
    Q_EMIT configChanged();
}

void KWin::KWinTabBoxConfigForm::onApplicationMode()
{
    m_config->setApplicationsMode(applicationMode());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onShowDesktopMode()
{
    m_config->setShowDesktopMode(showDesktopMode());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onSwitchingMode()
{
    m_config->setSwitchingMode(switchingMode());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onEffectCombo()
{
    const bool isAddonEffect = ui->effectCombo->currentData(AddonEffect).toBool();
    ui->effectConfigButton->setIcon(QIcon::fromTheme(isAddonEffect ? "view-preview" : "configure"));
    if (!ui->kcfg_ShowTabBox->isChecked()) {
        return;
    }
    ui->kcfg_HighlightWindows->setEnabled(isAddonEffect && m_isHighlightWindowsEnabled);

    m_config->setLayoutName(layoutName());
    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::onShortcutChanged(const QKeySequence &seq)
{
    QString actionName;
    if (sender()) {
        actionName = sender()->property("shortcutAction").toString();
    }
    if (actionName.isEmpty()) {
        return;
    }
    QAction *action = m_actionCollection->action(actionName);
    action->setShortcut(seq);

    Q_EMIT configChanged();
}

void KWinTabBoxConfigForm::updateUiFromConfig()
{
    setFilterScreen(static_cast<tabbox_config::ClientMultiScreenMode>(m_config->multiScreenMode()));
    setFilterDesktop(static_cast<tabbox_config::ClientDesktopMode>(m_config->desktopMode()));
    setFilterMinimization(static_cast<tabbox_config::ClientMinimizedMode>(m_config->minimizedMode()));
    setApplicationMode(static_cast<tabbox_config::ClientApplicationsMode>(m_config->applicationsMode()));
    setShowDesktopMode(static_cast<tabbox_config::ShowDesktopMode>(m_config->showDesktopMode()));
    setSwitchingModeChanged(static_cast<tabbox_config::ClientSwitchingMode>(m_config->switchingMode()));
    setLayoutName(m_config->layoutName());

    updateDefaultIndicators();
}

void KWinTabBoxConfigForm::setEnabledUi()
{
    m_isHighlightWindowsEnabled = !m_config->isHighlightWindowsImmutable();
    ui->kcfg_HighlightWindows->setEnabled(!m_config->isHighlightWindowsImmutable());

    ui->filterScreens->setEnabled(!m_config->isMultiScreenModeImmutable());
    ui->currentScreen->setEnabled(!m_config->isMultiScreenModeImmutable());
    ui->otherScreens->setEnabled(!m_config->isMultiScreenModeImmutable());

    ui->filterDesktops->setEnabled(!m_config->isDesktopModeImmutable());
    ui->currentDesktop->setEnabled(!m_config->isDesktopModeImmutable());
    ui->otherDesktops->setEnabled(!m_config->isDesktopModeImmutable());

    ui->filterMinimization->setEnabled(!m_config->isMinimizedModeImmutable());
    ui->visibleWindows->setEnabled(!m_config->isMinimizedModeImmutable());
    ui->hiddenWindows->setEnabled(!m_config->isMinimizedModeImmutable());

    ui->oneAppWindow->setEnabled(!m_config->isApplicationsModeImmutable());
    ui->showDesktop->setEnabled(!m_config->isShowDesktopModeImmutable());
    ui->switchingModeCombo->setEnabled(!m_config->isSwitchingModeImmutable());
    ui->effectCombo->setEnabled(!m_config->isLayoutNameImmutable());
}

void KWinTabBoxConfigForm::setDefaultIndicatorVisible(bool show)
{
    m_showDefaultIndicator = show;
    updateDefaultIndicators();
}

void KWinTabBoxConfigForm::updateDefaultIndicators()
{
    applyDefaultIndicator({ui->filterScreens, ui->currentScreen, ui->otherScreens},
                          m_config->multiScreenMode() == m_config->defaultMultiScreenModeValue());
    applyDefaultIndicator({ui->filterDesktops, ui->currentDesktop, ui->otherDesktops},
                          m_config->desktopMode() == m_config->defaultDesktopModeValue());
    applyDefaultIndicator({ui->filterMinimization, ui->visibleWindows, ui->hiddenWindows},
                          m_config->minimizedMode() == m_config->defaultMinimizedModeValue());
    applyDefaultIndicator({ui->oneAppWindow}, m_config->applicationsMode() == m_config->defaultApplicationsModeValue());
    applyDefaultIndicator({ui->showDesktop}, m_config->showDesktopMode() == m_config->defaultShowDesktopModeValue());
    applyDefaultIndicator({ui->switchingModeCombo}, m_config->switchingMode() == m_config->defaultSwitchingModeValue());
    applyDefaultIndicator({ui->effectCombo}, m_config->layoutName() == m_config->defaultLayoutNameValue());

    for (const auto &widget : {ui->scAll, ui->scAllReverse, ui->scCurrent, ui->scCurrentReverse}) {
        const QString actionName = widget->property("shortcutAction").toString();
        QAction *action = m_actionCollection->action(actionName);
        const bool isDefault = (action->shortcut() == m_actionCollection->defaultShortcut(action));
        applyDefaultIndicator({widget}, isDefault);
    }
}

void KWinTabBoxConfigForm::applyDefaultIndicator(QList<QWidget *> widgets, bool isDefault)
{
    for (auto widget : widgets) {
        widget->setProperty("_kde_highlight_neutral", m_showDefaultIndicator && !isDefault);
        widget->update();
    }
}

} // namespace

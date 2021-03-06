/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

/**
 * @file
 * @brief Application wide config to tune certain beahviours of the framework.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "Config.h"
#include "multisplitter/MultiSplitterConfig.h"
#include "multisplitter/Widget.h"
#include "DockRegistry_p.h"
#include "FrameworkWidgetFactory.h"
#include "Utils_p.h"

#include <QDebug>
#include <QOperatingSystemVersion>

namespace KDDockWidgets
{

class Config::Private
{
public:
    Private()
        : m_frameworkWidgetFactory(new DefaultWidgetFactory())
    {
    }

    ~Private()
    {
        delete m_frameworkWidgetFactory;
    }

    void fixFlags();

    QQmlEngine *m_qmlEngine = nullptr;
    DockWidgetFactoryFunc m_dockWidgetFactoryFunc = nullptr;
    MainWindowFactoryFunc m_mainWindowFactoryFunc = nullptr;
    TabbingAllowedFunc m_tabbingAllowedFunc = nullptr;
    FrameworkWidgetFactory *m_frameworkWidgetFactory;
    Flags m_flags = Flag_Default;
    qreal m_draggedWindowOpacity = Q_QNAN;
};

Config::Config()
    : d(new Private())
{
    d->fixFlags();

    // stuff in multisplitter/ can't include the framework widget factory, so set it here
    auto separatorCreator = [](Layouting::Widget *parent) {
        return Config::self().frameworkWidgetFactory()->createSeparator(parent);
    };

    Layouting::Config::self().setSeparatorFactoryFunc(separatorCreator);
}

Config& Config::self()
{
    static Config config;
    return config;
}

Config::~Config()
{
    delete d;
}

Config::Flags Config::flags() const
{
    return d->m_flags;
}

void Config::setFlags(Flags f)
{
    auto dr = DockRegistry::self();
    if (!dr->isEmpty(/*excludeBeingDeleted=*/ true)) {
        qWarning() << Q_FUNC_INFO << "Only use this function at startup before creating any DockWidget or MainWindow"
                   << "; These are already created: " << dr->mainWindowsNames()
                   << dr->dockWidgetNames() << dr->floatingWindows();
        return;
    }

    d->m_flags = f;
    d->fixFlags();

    auto multisplitterFlags = Layouting::Config::self().flags();
    multisplitterFlags.setFlag(Layouting::Config::Flag::LazyResize, d->m_flags & Flag_LazyResize);
    Layouting::Config::self().setFlags(multisplitterFlags);
}

void Config::setDockWidgetFactoryFunc(DockWidgetFactoryFunc func)
{
    d->m_dockWidgetFactoryFunc = func;
}

DockWidgetFactoryFunc Config::dockWidgetFactoryFunc() const
{
    return d->m_dockWidgetFactoryFunc;
}

void Config::setMainWindowFactoryFunc(MainWindowFactoryFunc func)
{
    d->m_mainWindowFactoryFunc = func;
}

MainWindowFactoryFunc Config::mainWindowFactoryFunc() const
{
    return d->m_mainWindowFactoryFunc;
}

void Config::setFrameworkWidgetFactory(FrameworkWidgetFactory *wf)
{
    Q_ASSERT(wf);
    delete d->m_frameworkWidgetFactory;
    d->m_frameworkWidgetFactory = wf;
}

FrameworkWidgetFactory *Config::frameworkWidgetFactory() const
{
    return d->m_frameworkWidgetFactory;
}

int Config::separatorThickness() const
{
    return Layouting::Config::self().separatorThickness();
}

void Config::setSeparatorThickness(int value)
{
    if (!DockRegistry::self()->isEmpty(/*excludeBeingDeleted=*/ true)) {
        qWarning() << Q_FUNC_INFO << "Only use this function at startup before creating any DockWidget or MainWindow";
        return;
    }

    Layouting::Config::self().setSeparatorThickness(value);
}

void Config::setDraggedWindowOpacity(qreal opacity)
{
    d->m_draggedWindowOpacity = opacity;
}

qreal Config::draggedWindowOpacity() const
{
    return d->m_draggedWindowOpacity;
}

void Config::setTabbingAllowedFunc(TabbingAllowedFunc func)
{
    d->m_tabbingAllowedFunc = func;
}

TabbingAllowedFunc Config::tabbingAllowedFunc() const
{
    return d->m_tabbingAllowedFunc;
}

void Config::setQmlEngine(QQmlEngine *qmlEngine)
{
    if (d->m_qmlEngine) {
        qWarning() << Q_FUNC_INFO << "Already has QML engine";
        return;
    }

    d->m_qmlEngine = qmlEngine;
}

QQmlEngine *Config::qmlEngine() const
{
    return d->m_qmlEngine;
}

void Config::Private::fixFlags()
{
#if defined(Q_OS_WIN)
    if (QOperatingSystemVersion::current().majorVersion() < 10) {
        // Aero-snap requires Windows 10
        m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
    } else {
        // Unconditional now
        m_flags |= Flag_AeroSnapWithClientDecos;
    }

    // These are mutually exclusive:
    if ((m_flags & Flag_AeroSnapWithClientDecos) && (m_flags & Flag_NativeTitleBar)) {
        // We're either using native or client decorations, let's use native.
        m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
    }
#elif defined(Q_OS_MACOS)
    // Not supported on macOS:
    m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
#else
    if (KDDockWidgets::isWayland()) {
        // Native title bar is forced on Wayland. Needed for moving the window.
        // The inner KDDW title bar is used for DnD.
        m_flags |= Flag_NativeTitleBar;
    } else {
        // Not supported on linux/X11
        // On Linux, dragging the title bar of a window doesn't generate NonClientMouseEvents
        // at least with KWin anyway. We can make this more granular and allow it for other
        // X11 window managers
        m_flags = m_flags & ~Flag_NativeTitleBar;
        m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
    }
#endif

#if (!defined(Q_OS_WIN) && !defined(Q_OS_MACOS)) || defined(KDDOCKWIDGETS_QTQUICK)
    // QtQuick doesn't support AeroSnap yet. Some problem with the native events not being received...
    m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
#endif


#if defined(DOCKS_DEVELOPER_MODE)
    // We allow to disable aero-snap during development
    if (m_flags & Flag_internal_NoAeroSnap) {
        // The only way to disable AeroSnap
        m_flags = m_flags & ~Flag_AeroSnapWithClientDecos;
    }
#endif
}

}

/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

/**
 * @file
 * @brief Represents a dock widget.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_DOCKWIDGET_QUICK_H
#define KD_DOCKWIDGET_QUICK_H

#include "DockWidgetBase.h"

QT_BEGIN_NAMESPACE
class QCloseEvent;
QT_END_NAMESPACE

namespace KDDockWidgets {

/**
 * @brief Represents a dock widget.
 *
 * Most of the interface lives in DockWidgetBase, to facilitate sharing with QtQuick.
 */
class DOCKS_EXPORT DockWidgetQuick : public DockWidgetBase
{
    Q_OBJECT
public:
    /**
     * @brief constructs a new DockWidget
     * @param name the name of the dockwidget, should be unique. Use title for user visible text.
     * @param options optional options controlling behaviour
     * @param parent optional QWidget parent, for ownership purposes
     *
     * There's no parent argument. The DockWidget is either parented to FloatingWindow or MainWindow
     * when visible, or stays without a parent when hidden.
     */
    explicit DockWidgetQuick(const QString &uniqueName, Options options = {});

    ///@brief destructor
    ~DockWidgetQuick() override;

    /// Sets the DockWidget's guest item
    /// Similar to DockWidgetBase::setWidget(QQuickItem*)
    void setWidget(const QString &qmlFilename);

    /// @reimp
    void setWidget(QWidgetOrQuick *widget) override;

    /// @reimp
    QSize minimumSize() const override;

    /// @reimp
    QSize maximumSize() const override;

protected:
    bool event(QEvent *e) override;

private:
    class Private;
    Private *const d;
};

}

#endif

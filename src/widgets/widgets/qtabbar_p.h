/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTABBAR_P_H
#define QTABBAR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qtabbar.h"
#include "private/qwidget_p.h"

#include <qicon.h>
#include <qtoolbutton.h>
#include <qdebug.h>
#if QT_CONFIG(animation)
#include <qvariantanimation.h>
#endif

#define ANIMATION_DURATION 250

#include <qstyleoption.h>

QT_REQUIRE_CONFIG(tabbar);

QT_BEGIN_NAMESPACE


//------------------------------------------------------------------
// Autodesk 3ds Max addition: Tabs menu button
// Adds a new tool button to the tab left / right scroll buttons
// which opens up a quick select menu containing all tabs.
//------------------------------------------------------------------
class TabsMenuBtn : public QToolButton
{
    Q_OBJECT

public:
    TabsMenuBtn( QTabBar* parent = nullptr );

    void tabOrderChanged();
    void updateTabsMenu();

protected:
    void paintEvent( QPaintEvent* evt ) Q_DECL_OVERRIDE;

public slots:
    void tabsMenuActionTriggered( QAction* action );
    void tabsMenuAboutToShow();
    void currentTabChanged();

private:
    QMenu* mTabsMenu = nullptr;
    QTabBar* mTabBar = nullptr;
    bool mTabOrderDirty = true;
    bool mCurTabDirty = true;
};


class QMovableTabWidget : public QWidget
{
public:
    explicit QMovableTabWidget(QWidget *parent = nullptr);
    void setPixmap(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    QPixmap m_pixmap;
};

class Q_WIDGETS_EXPORT QTabBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QTabBar)
public:
    QTabBarPrivate()
        :currentIndex(-1), pressedIndex(-1), firstVisible(0), lastVisible(-1), shape(QTabBar::RoundedNorth), layoutDirty(false),
        drawBase(true), scrollOffset(0), hoverIndex(-1), elideModeSetByUser(false), useScrollButtonsSetByUser(false), expanding(true), closeButtonOnTabs(false),
        selectionBehaviorOnRemove(QTabBar::SelectRightTab), paintWithOffsets(true), movable(false),
        dragInProgress(false), documentMode(false), autoHide(false), changeCurrentOnDrag(false),
        switchTabCurrentIndex(-1), switchTabTimerId(0), movingTab(nullptr)
        {}

    int currentIndex;
    int pressedIndex;
    int firstVisible;
    int lastVisible;
    QTabBar::Shape shape;
    bool layoutDirty;
    bool drawBase;
    int scrollOffset;
    int lineCount = 0;
    mutable struct {
        int width;
        int height;
    } heightForWidthCache = { -1, -1 };

    //------------------------------------------------------------------
    // Autodesk 3ds Max addition: Tabs menu button
    // Specifies additional flags that can be used to show / hide the
    // left & right tab scroll buttons in combination with 
    // the new tabs menu button.
    //------------------------------------------------------------------
    enum TabScrollOption {
        TabScrollBtnsShown = 0x00001,
        TabMenuBtnShown = 0x00002
    };

    Q_DECLARE_FLAGS( TabScrollOptions, TabScrollOption )


    struct Tab {
        inline Tab(const QIcon &ico, const QString &txt)
            : enabled(true) , visible(true), shortcutId(0), text(txt), icon(ico),
            leftWidget(nullptr), rightWidget(nullptr), lastTab(-1), dragOffset(0)
#if QT_CONFIG(animation)
            , animation(nullptr)
#endif // animation
        {}
        bool operator==(const Tab &other) const { return &other == this; }
        bool enabled;
        bool visible;
        int shortcutId;
        QString text;
#ifndef QT_NO_TOOLTIP
        QString toolTip;
#endif
#if QT_CONFIG(whatsthis)
        QString whatsThis;
#endif
        QIcon icon;
        QRect rect;
        QRect minRect;
        QRect maxRect;

        QColor textColor;
        QVariant data;
        QWidget *leftWidget;
        QWidget *rightWidget;
        int lastTab;
        int dragOffset;
#ifndef QT_NO_ACCESSIBILITY
        QString accessibleName;
#endif
        int row = -1;
        int rowIndex = -1;
        bool isLastTabInRow = false;

#if QT_CONFIG(animation)
        ~Tab() { delete animation; }
        struct TabBarAnimation : public QVariantAnimation {
            TabBarAnimation(Tab *t, QTabBarPrivate *_priv) : tab(t), priv(_priv)
            { setEasingCurve(QEasingCurve::InOutQuad); }

            void updateCurrentValue(const QVariant &current) override;

            void updateState(State newState, State) override;
        private:
            //these are needed for the callbacks
            Tab *tab;
            QTabBarPrivate *priv;
        } *animation;

        void startAnimation(QTabBarPrivate *priv, int duration) {
            if (!priv->isAnimated()) {
                priv->moveTabFinished(priv->tabList.indexOf(*this));
                return;
            }
            if (!animation)
                animation = new TabBarAnimation(this, priv);
            animation->setStartValue(dragOffset);
            animation->setEndValue(0);
            animation->setDuration(duration);
            animation->start();
        }
#else
        void startAnimation(QTabBarPrivate *priv, int duration)
        { Q_UNUSED(duration); priv->moveTabFinished(priv->tabList.indexOf(*this)); }
#endif // animation
    };
    QList<Tab> tabList;
    mutable QHash<QString, QSize> textSizes;

    void calculateFirstLastVisible(int index, bool visible, bool remove);
    int selectNewCurrentIndexFrom(int currentIndex);
    int calculateNewPosition(int from, int to, int index) const;
    void slide(int from, int to);
    void init();

    Tab *at(int index);
    const Tab *at(int index) const;

    int indexAtPos(const QPoint &p) const;

    inline bool isAnimated() const { Q_Q(const QTabBar); return q->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, q) > 0; }
    inline bool validIndex(int index) const { return index >= 0 && index < tabList.count(); }
    void setCurrentNextEnabledIndex(int offset);

    QToolButton* rightB; // right or bottom
    QToolButton* leftB; // left or top
    TabsMenuBtn* tabsMenuBtn = nullptr; // Adsk 3ds Max: Tab switch menu button

    void _q_scrollTabs();
    void _q_closeTab();
    void moveTab(int index, int offset);
    void moveTabFinished(int index);
    QRect hoverRect;
    int hoverIndex;

    void refresh();
    void layoutTabs();
    void layoutWidgets(int start = 0);
    void layoutTab(int index);
    void updateMacBorderMetrics();
    bool isTabInMacUnifiedToolbarArea() const;
    void setupMovableTab();
    void autoHideTabs();
    QRect normalizedScrollRect(int index = -1);
    int hoveredTabIndex() const;

    void initBasicStyleOption(QStyleOptionTab *option, int tabIndex) const;

    void makeVisible(int index);
    QSize iconSize;
    Qt::TextElideMode elideMode;
    bool elideModeSetByUser;
    bool useScrollButtons;
    bool useScrollButtonsSetByUser;
    TabScrollOptions tabScrollBtnOptions = TabMenuBtnShown; // Adsk 3ds Max

    bool expanding;
    bool closeButtonOnTabs;
    QTabBar::SelectionBehavior selectionBehaviorOnRemove;

    QPoint dragStartPosition;
    bool paintWithOffsets;
    bool movable;
    bool dragInProgress;
    bool documentMode;
    bool autoHide;
    bool changeCurrentOnDrag;
    bool multiRow = false; // Adsk 3ds Max

    int switchTabCurrentIndex;
    int switchTabTimerId;

    QMovableTabWidget *movingTab;
    // shared by tabwidget and qtabbar
    static void initStyleBaseOption(QStyleOptionTabBarBase *optTabBase, QTabBar *tabbar, QSize size)
    {
        QStyleOptionTab tabOverlap;
        tabOverlap.shape = tabbar->shape();
        int overlap = tabbar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, tabbar);
        QWidget *theParent = tabbar->parentWidget();
        optTabBase->init(tabbar);
        optTabBase->shape = tabbar->shape();
        optTabBase->documentMode = tabbar->documentMode();
        if (theParent && overlap > 0) {
            QRect rect;
            switch (tabOverlap.shape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                rect.setRect(0, size.height()-overlap, size.width(), overlap);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                rect.setRect(0, 0, size.width(), overlap);
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                rect.setRect(0, 0, overlap, size.height());
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                rect.setRect(size.width() - overlap, 0, overlap, size.height());
                break;
            }
            optTabBase->rect = rect;
        }
    }

    void killSwitchTabTimer();

};

QT_END_NAMESPACE

#endif

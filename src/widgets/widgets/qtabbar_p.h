// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <utility>

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
    : layoutDirty(false), drawBase(true), elideModeSetByUser(false), useScrollButtons(false),
      useScrollButtonsSetByUser(false), expanding(true), closeButtonOnTabs(false),
      paintWithOffsets(true), movable(false), dragInProgress(false), documentMode(false),
      autoHide(false), changeCurrentOnDrag(false)
    {}
    ~QTabBarPrivate()
    {
        qDeleteAll(tabList);
    }

    QRect hoverRect;
    QPoint dragStartPosition;
    QPoint mousePosition = {-1, -1};
#if QT_CONFIG(wheelevent)
    QPoint accumulatedAngleDelta;
#endif
    QSize iconSize;
    QToolButton* rightB = nullptr; // right or bottom
    QToolButton* leftB = nullptr; // left or top
    TabsMenuBtn* tabsMenuBtn = nullptr; // Adsk 3ds Max: Tab switch menu button
    QMovableTabWidget *movingTab = nullptr;
    int hoverIndex = -1;
    int switchTabCurrentIndex = -1;
    int switchTabTimerId = 0;
    Qt::TextElideMode elideMode = Qt::ElideNone;
    QTabBar::SelectionBehavior selectionBehaviorOnRemove = QTabBar::SelectRightTab;
    QTabBar::Shape shape = QTabBar::RoundedNorth;
    Qt::MouseButtons mouseButtons = Qt::NoButton;

    int currentIndex = -1;
    int pressedIndex = -1;
    int firstVisible = 0;
    int lastVisible = -1;
    int scrollOffset = 0;

    bool layoutDirty : 1;
    bool drawBase : 1;
    bool elideModeSetByUser : 1;
    bool useScrollButtons : 1;
    bool useScrollButtonsSetByUser : 1;
    bool expanding : 1;
    bool closeButtonOnTabs : 1;
    bool paintWithOffsets : 1;
    bool movable : 1;
    bool dragInProgress : 1;
    bool documentMode : 1;
    bool autoHide : 1;
    bool changeCurrentOnDrag : 1;

    bool multiRow = false; // Adsk 3ds Max
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

    TabScrollOptions tabScrollBtnOptions = TabMenuBtnShown; // Adsk 3ds Max

    struct Tab {
        inline Tab(const QIcon &ico, const QString &txt)
        : text(txt), icon(ico), enabled(true), visible(true)
        {
        }
        /*
            Tabs are managed by instance; they are not the same even
            if all properties are the same.
        */
        Q_DISABLE_COPY_MOVE(Tab);

        QString text;
#if QT_CONFIG(tooltip)
        QString toolTip;
#endif
#if QT_CONFIG(whatsthis)
        QString whatsThis;
#endif
#if QT_CONFIG(accessibility)
        QString accessibleName;
#endif
        QIcon icon;
        QRect rect;
        QRect minRect;
        QRect maxRect;

        QColor textColor;
        QVariant data;
        QWidget *leftWidget = nullptr;
        QWidget *rightWidget = nullptr;
        int shortcutId = 0;
        int lastTab = -1;
        int dragOffset = 0;
        uint enabled : 1;
        uint visible : 1;

        int row = -1;
        int rowIndex = -1;
        bool isLastTabInRow = false;

#if QT_CONFIG(animation)
        struct TabBarAnimation : public QVariantAnimation {
            TabBarAnimation(Tab *t, QTabBarPrivate *_priv) : tab(t), priv(_priv)
            { setEasingCurve(QEasingCurve::InOutQuad); }

            void updateCurrentValue(const QVariant &current) override;

            void updateState(State newState, State) override;
        private:
            //these are needed for the callbacks
            Tab *tab;
            QTabBarPrivate *priv;
        };
        std::unique_ptr<TabBarAnimation> animation;

        void startAnimation(QTabBarPrivate *priv, int duration) {
            if (!priv->isAnimated()) {
                priv->moveTabFinished(priv->tabList.indexOf(this));
                return;
            }
            if (!animation)
                animation = std::make_unique<TabBarAnimation>(this, priv);
            animation->setStartValue(dragOffset);
            animation->setEndValue(0);
            animation->setDuration(duration);
            animation->start();
        }
#else
        void startAnimation(QTabBarPrivate *priv, int duration)
        { Q_UNUSED(duration); priv->moveTabFinished(priv->tabList.indexOf(this)); }
#endif // animation
    };
    QList<Tab*> tabList;
    mutable QHash<QString, QSize> textSizes;

    void calculateFirstLastVisible(int index, bool visible, bool remove);
    int selectNewCurrentIndexFrom(int currentIndex);
    int calculateNewPosition(int from, int to, int index) const;
    void slide(int from, int to);
    void init();

    inline Tab *at(int index) { return tabList.value(index, nullptr); }
    inline const Tab *at(int index) const { return tabList.value(index, nullptr); }

    int indexAtPos(const QPoint &p) const;

    inline bool isAnimated() const { Q_Q(const QTabBar); return q->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, q) > 0; }
    inline bool validIndex(int index) const { return index >= 0 && index < tabList.size(); }
    void setCurrentNextEnabledIndex(int offset);

    void _q_scrollTabs();
    void _q_closeTab();
    void moveTab(int index, int offset);
    void moveTabFinished(int index);

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

    // shared by tabwidget and qtabbar
    static void initStyleBaseOption(QStyleOptionTabBarBase *optTabBase, QTabBar *tabbar, QSize size)
    {
        QStyleOptionTab tabOverlap;
        tabOverlap.shape = tabbar->shape();
        int overlap = tabbar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &tabOverlap, tabbar);
        QWidget *theParent = tabbar->parentWidget();
        optTabBase->initFrom(tabbar);
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

constexpr inline bool verticalTabs(QTabBar::Shape shape) noexcept
{
    return shape == QTabBar::RoundedWest
            || shape == QTabBar::RoundedEast
            || shape == QTabBar::TriangularWest
            || shape == QTabBar::TriangularEast;
}

QT_END_NAMESPACE

#endif

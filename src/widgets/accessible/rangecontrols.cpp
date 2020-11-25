/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "rangecontrols_p.h"

#if QT_CONFIG(slider)
#include <qslider.h>
#endif
#if QT_CONFIG(dial)
#include <qdial.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#if QT_CONFIG(scrollbar)
#include <qscrollbar.h>
#endif
#include <qstyle.h>
#include <qstyleoption.h>
#include <qdebug.h>
#include <qglobal.h>
#if QT_CONFIG(lineedit)
#include <QtWidgets/qlineedit.h>
#endif
#include <qmath.h>
#include <private/qmath_p.h>

#include "simplewidgets_p.h" // let spinbox use line edit's interface

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

#if QT_CONFIG(spinbox)

class QAccessibleSpinBoxButton : public QAccessibleInterface
{
    public:
    
    QAccessibleSpinBoxButton( QAccessibleInterface* parent, QAbstractSpinBox* spinBox, QStyle::SubControl which )
    :   m_Parent( parent ),
        m_SpinBox( spinBox ),
        m_Which( which )
    {}

    ~QAccessibleSpinBoxButton() { m_SpinBox = nullptr; }

    // check for valid pointers
    bool isValid() const override { return m_SpinBox != nullptr; }
    QObject *object() const override { return nullptr; }

    // relations
    QAccessibleInterface *childAt(int, int) const override { return nullptr; }

    // navigation, hierarchy
    QAccessibleInterface* parent() const override { return m_Parent; }
    QAccessibleInterface* child(int) const override { return nullptr; }
    int childCount() const override { return 0; }
    int indexOfChild(const QAccessibleInterface*) const override { return -1; }
    
    // properties and state
    QString text(QAccessible::Text t) const override
    {
        if (t == QAccessible::Text::Name)
        {
            switch (m_Which)
            {
            case QStyle::SC_SpinBoxUp:
                return "Up";
            case QStyle::SC_SpinBoxDown:
                return "Down";
            }
        }
        return "";
    }
    void setText(QAccessible::Text, const QString &) override
    {
        Q_ASSERT(0);
    }

    QRect rect() const override
    {
        if ( m_SpinBox )
        {
            QStyleOptionSpinBox opt; 
            opt.initFrom( m_SpinBox );
            QRect rect = m_SpinBox->style()->subControlRect( QStyle::CC_SpinBox, &opt, m_Which, m_SpinBox );
            rect.translate(m_SpinBox->mapToGlobal(QPoint(0, 0)));
            return rect;
        }
        return QRect();
    }

    QAccessible::Role role() const override { return QAccessible::Button; }

    QAccessible::State state() const override 
    { 
        QAccessible::State state;
        if ( m_SpinBox )
        {
             if (m_SpinBox->testAttribute(Qt::WA_WState_Visible) == false)
             {
                state.invisible = true;
             }
            if (!m_SpinBox->isEnabled())
            {
                state.disabled = true;
            }
            else
            {
                QStyleOptionSpinBox opt; 
                opt.initFrom( m_SpinBox );
                if (( m_Which == QStyle::SC_SpinBoxUp && !opt.stepEnabled.testFlag( QAbstractSpinBox::StepUpEnabled ) ) || 
                    ( m_Which == QStyle::SC_SpinBoxDown && !opt.stepEnabled.testFlag( QAbstractSpinBox::StepDownEnabled ) ) )
                {
                    state.disabled = true; 
                }
            }
        }
        return state;
    }

    private:
    QAccessibleInterface* m_Parent;
    QAbstractSpinBox* m_SpinBox;
    QStyle::SubControl m_Which;
};


QAccessibleAbstractSpinBox::QAccessibleAbstractSpinBox(QWidget *w)
: QAccessibleWidget(w, QAccessible::SpinBox),
    lineEdit(nullptr), buttonUp( nullptr ), buttonDown( nullptr )
{
    Q_ASSERT(abstractSpinBox());
}

QAccessibleAbstractSpinBox::~QAccessibleAbstractSpinBox()
{
    if ( buttonUp )
    {
        QAccessible::deleteAccessibleInterface( QAccessible::uniqueId( buttonUp ) );
        buttonUp = nullptr;
    }
    if ( buttonDown )
    {
        QAccessible::deleteAccessibleInterface( QAccessible::uniqueId( buttonDown ) );
        buttonDown = nullptr;
    }
    delete lineEdit;
}

/*!
    Returns the underlying QAbstractSpinBox.
*/
QAbstractSpinBox *QAccessibleAbstractSpinBox::abstractSpinBox() const
{
    return qobject_cast<QAbstractSpinBox*>(object());
}

QAccessibleInterface *QAccessibleAbstractSpinBox::lineEditIface() const
{
#if QT_CONFIG(lineedit)
    // QAccessibleLineEdit is only used to forward the text functions
    if (!lineEdit)
        lineEdit = new QAccessibleLineEdit(abstractSpinBox()->lineEdit());
    return lineEdit;
#else
    return nullptr;
#endif
}

QAccessibleInterface *QAccessibleAbstractSpinBox::buttonUpIface() const
{
    if ( !buttonUp ) 
    {
        QAbstractSpinBox* sp = abstractSpinBox();
        if ( sp->buttonSymbols() != QAbstractSpinBox::NoButtons )
        {
            QAccessibleSpinBoxButton* up = new QAccessibleSpinBoxButton( const_cast<QAccessibleAbstractSpinBox*>(this), sp, QStyle::SC_SpinBoxUp );
            QAccessible::registerAccessibleInterface( up );
            buttonUp = up;
        }
    }
    return buttonUp;
}

QAccessibleInterface *QAccessibleAbstractSpinBox::buttonDownIface() const 
{
    if ( !buttonDown )
    {
        QAbstractSpinBox* sp = abstractSpinBox();
        if ( sp->buttonSymbols() != QAbstractSpinBox::NoButtons )
        {
            QAccessibleSpinBoxButton* down = new QAccessibleSpinBoxButton( const_cast<QAccessibleAbstractSpinBox*>(this), sp, QStyle::SC_SpinBoxDown );
            QAccessible::registerAccessibleInterface( down );
            buttonDown = down; 
        }
    }
    return buttonDown;
}

QString QAccessibleAbstractSpinBox::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return abstractSpinBox()->text();
    return QAccessibleWidget::text(t);
}

int QAccessibleAbstractSpinBox::childCount() const
{
    QAbstractSpinBox* sp = abstractSpinBox();
    if ( sp->buttonSymbols() == QAbstractSpinBox::NoButtons ) return 1;
    return 3;
}

QAccessibleInterface* QAccessibleAbstractSpinBox::child(int idx) const
{
    if (idx == 0) return QAccessible::queryAccessibleInterface( abstractSpinBox()->lineEdit() );
    if (idx == 1) return buttonUpIface();
    if (idx == 2) return buttonDownIface();
    return nullptr;
}

int QAccessibleAbstractSpinBox::indexOfChild(const QAccessibleInterface *child) const
{
    if ( child )
    {
        if ( child->object() == abstractSpinBox()->lineEdit() )
        {
            return 0;
        }
        if ( buttonUp == child ) return 1;
        if ( buttonDown == child ) return 2;
    }
    return -1;
}

void *QAccessibleAbstractSpinBox::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QVariant QAccessibleAbstractSpinBox::currentValue() const
{
    return abstractSpinBox()->property("value");
}

void QAccessibleAbstractSpinBox::setCurrentValue(const QVariant &value)
{
    abstractSpinBox()->setProperty("value", value);
}

QVariant QAccessibleAbstractSpinBox::maximumValue() const
{
    return abstractSpinBox()->property("maximum");
}

QVariant QAccessibleAbstractSpinBox::minimumValue() const
{
    return abstractSpinBox()->property("minimum");
}

QVariant QAccessibleAbstractSpinBox::minimumStepSize() const
{
    return abstractSpinBox()->property("stepSize");
}

void QAccessibleAbstractSpinBox::addSelection(int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->addSelection(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::attributes(int offset, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->attributes(offset, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::cursorPosition() const
{
    return lineEditIface()->textInterface()->cursorPosition();
}

QRect QAccessibleAbstractSpinBox::characterRect(int offset) const
{
    return lineEditIface()->textInterface()->characterRect(offset);
}

int QAccessibleAbstractSpinBox::selectionCount() const
{
    return lineEditIface()->textInterface()->selectionCount();
}

int QAccessibleAbstractSpinBox::offsetAtPoint(const QPoint &point) const
{
    return lineEditIface()->textInterface()->offsetAtPoint(point);
}

void QAccessibleAbstractSpinBox::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    lineEditIface()->textInterface()->selection(selectionIndex, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::text(int startOffset, int endOffset) const
{
    return lineEditIface()->textInterface()->text(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textBeforeOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAfterOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAtOffset(offset, boundaryType, startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::removeSelection(int selectionIndex)
{
    lineEditIface()->textInterface()->removeSelection(selectionIndex);
}

void QAccessibleAbstractSpinBox::setCursorPosition(int position)
{
    lineEditIface()->textInterface()->setCursorPosition(position);
}

void QAccessibleAbstractSpinBox::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->setSelection(selectionIndex, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::characterCount() const
{
    return lineEditIface()->textInterface()->characterCount();
}

void QAccessibleAbstractSpinBox::scrollToSubstring(int startIndex, int endIndex)
{
    lineEditIface()->textInterface()->scrollToSubstring(startIndex, endIndex);
}

void QAccessibleAbstractSpinBox::deleteText(int startOffset, int endOffset)
{
    lineEditIface()->editableTextInterface()->deleteText(startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::insertText(int offset, const QString &text)
{
    lineEditIface()->editableTextInterface()->insertText(offset, text);
}

void QAccessibleAbstractSpinBox::replaceText(int startOffset, int endOffset, const QString &text)
{
    lineEditIface()->editableTextInterface()->replaceText(startOffset, endOffset, text);
}


/*!
  \class QAccessibleSpinBox
  \brief The QAccessibleSpinBox class implements the QAccessibleInterface for spinbox widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleSpinWidget object for \a w.
*/
QAccessibleSpinBox::QAccessibleSpinBox(QWidget *w)
: QAccessibleAbstractSpinBox(w)
{
    Q_ASSERT(spinBox());
    addControllingSignal(QLatin1String("valueChanged(int)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QSpinBox.
*/
QSpinBox *QAccessibleSpinBox::spinBox() const
{
    return qobject_cast<QSpinBox*>(object());
}


// ================================== QAccessibleDoubleSpinBox ==================================
QAccessibleDoubleSpinBox::QAccessibleDoubleSpinBox(QWidget *widget)
    : QAccessibleAbstractSpinBox(widget)
{
    Q_ASSERT(qobject_cast<QDoubleSpinBox *>(widget));
    addControllingSignal(QLatin1String("valueChanged(double)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QDoubleSpinBox.
*/
QDoubleSpinBox *QAccessibleDoubleSpinBox::doubleSpinBox() const
{
    return static_cast<QDoubleSpinBox*>(object());
}

QString QAccessibleDoubleSpinBox::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return doubleSpinBox()->textFromValue(doubleSpinBox()->value());
    return QAccessibleWidget::text(textType);
}

#endif // QT_CONFIG(spinbox)

#if QT_CONFIG(scrollbar)
/*!
  \class QAccessibleScrollBar
  \brief The QAccessibleScrollBar class implements the QAccessibleInterface for scroll bars.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleScrollBar::QAccessibleScrollBar(QWidget *w)
: QAccessibleAbstractSlider(w, QAccessible::ScrollBar)
{
    Q_ASSERT(scrollBar());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the scroll bar. */
QScrollBar *QAccessibleScrollBar::scrollBar() const
{
    return qobject_cast<QScrollBar*>(object());
}

QString QAccessibleScrollBar::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(scrollBar()->value());
    return QAccessibleAbstractSlider::text(t);
}

#endif // QT_CONFIG(scrollbar)

#if QT_CONFIG(slider)
/*!
  \class QAccessibleSlider
  \brief The QAccessibleSlider class implements the QAccessibleInterface for sliders.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleSlider::QAccessibleSlider(QWidget *w)
: QAccessibleAbstractSlider(w)
{
    Q_ASSERT(slider());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the slider. */
QSlider *QAccessibleSlider::slider() const
{
    return qobject_cast<QSlider*>(object());
}

QString QAccessibleSlider::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(slider()->value());

    return QAccessibleAbstractSlider::text(t);
}

QAccessibleAbstractSlider::QAccessibleAbstractSlider(QWidget *w, QAccessible::Role r)
    : QAccessibleWidget(w, r)
{
    Q_ASSERT(qobject_cast<QAbstractSlider *>(w));
}

void *QAccessibleAbstractSlider::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QVariant QAccessibleAbstractSlider::currentValue() const
{
    return abstractSlider()->value();
}

void QAccessibleAbstractSlider::setCurrentValue(const QVariant &value)
{
    abstractSlider()->setValue(value.toInt());
}

QVariant QAccessibleAbstractSlider::maximumValue() const
{
    return abstractSlider()->maximum();
}

QVariant QAccessibleAbstractSlider::minimumValue() const
{
    return abstractSlider()->minimum();
}

QVariant QAccessibleAbstractSlider::minimumStepSize() const
{
    return abstractSlider()->singleStep();
}

QAbstractSlider *QAccessibleAbstractSlider::abstractSlider() const
{
    return static_cast<QAbstractSlider *>(object());
}

#endif // QT_CONFIG(slider)

#if QT_CONFIG(dial)
// ======================================= QAccessibleDial ======================================
QAccessibleDial::QAccessibleDial(QWidget *widget)
    : QAccessibleAbstractSlider(widget, QAccessible::Dial)
{
    Q_ASSERT(qobject_cast<QDial *>(widget));
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

QString QAccessibleDial::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return QString::number(dial()->value());

    return QAccessibleAbstractSlider::text(textType);
}

QDial *QAccessibleDial::dial() const
{
    return static_cast<QDial*>(object());
}
#endif // QT_CONFIG(dial)

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

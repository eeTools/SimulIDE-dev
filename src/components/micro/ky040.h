/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "e-element.h"
#include "component.h"
#include "dialwidget.h"

class LibraryItem;
class QToolButton;
class IoPin;
class CustomDial;

class KY040 : public Component, public eElement
{
    public:
        KY040( QString type, QString id );
        ~KY040();

        static Component* construct( QString type, QString id );
        static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;
        virtual void runEvent() override;

        int steps() { return m_steps; }
        void setSteps( int s );

        virtual void setLinkedValue( double v, int i=0 ) override;

        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;
        
    public slots:
        void onbuttonchanged();

    private:
        int m_steps;
        int m_prevDialVal;
        int m_delta;
        uint64_t m_stepDelta;
        int m_posA;
        int m_posB;

        bool m_stateA;
        bool m_stateB;
        
        bool m_changed;

        CustomDial* m_dial;
        QGraphicsProxyWidget* m_proxy;
        
        QToolButton* m_button;
        QGraphicsProxyWidget* m_proxy_button;

        IoPin* m_pinA;
        IoPin* m_pinB;
        IoPin* m_sw;
};

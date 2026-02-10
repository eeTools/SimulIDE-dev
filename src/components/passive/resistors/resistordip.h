/***************************************************************************
 *   Copyright (C) 2016 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"

class LibraryItem;
class eResistor;
class Pin;

class ResistorDip : public Component, public eElement
{
    public:
        ResistorDip( QString type, QString id );
        ~ResistorDip();

 static Component* construct( QString type, QString id );
 static LibraryItem *libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;

        int size() { return m_size; }
        void setSize( int size );

        double resistance() { return m_resistance; }
        void setResistance( double resist );

        bool pullUp() { return m_pullUp; }
        void setPullUp( bool p );

        double puVolt() { return m_puVolt; }
        void setPuVolt( double pv ) { m_puVolt = pv; update(); }

        void createResistors( int c );
        void deleteResistors( int d );

        virtual void remove() override;
        
        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    protected slots:
        virtual void slotProperties() override;

    private:
        void updtProperties();

        double m_resistance;
        int m_size;

        bool m_pullUp;
        double m_puVolt;

        std::vector<Pin*> m_pin;
        std::vector<eResistor*> m_resistor;

        static eNode m_puEnode;
};

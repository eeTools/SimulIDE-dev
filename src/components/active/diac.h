/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "component.h"
#include "e-element.h"

class LibraryItem;
class eDiode;
class eResistor;

class Diac : public Component, public eElement
{
    public:
        Diac( QString type, QString id );
        ~Diac();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        void initialize() override;
        void stamp() override;
        void updateStep() override;
        void voltChanged() override;

        double resOn() { return m_resOn; }
        void setResOn( double r ) { m_resOn = r; m_changed = true; }

        double resOff() { return m_resOff; }
        void setResOff( double r ) { m_resOff = r; m_changed = true; }

        double brkVolt() { return m_brkVolt; }
        void setBrkVolt( double v ) { m_brkVolt = v; m_changed = true; }

        double holdCurr() { return m_holdCurr; }
        void setHoldCurr( double v ) { m_holdCurr = v; m_changed = true; }

        void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    private:
        double m_resOn;
        double m_resOff;
        double m_brkVolt;
        double m_holdCurr;

        bool m_state;

        eNode* m_midEnode;
        eResistor* m_resistor;

        eDiode* m_diode1;
        eDiode* m_diode2;
};

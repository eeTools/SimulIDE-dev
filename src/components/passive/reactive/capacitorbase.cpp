/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "capacitorbase.h"
#include "simulator.h"
#include "pin.h"

#include "doubleprop.h"

#define tr(str) simulideTr("CapacitorBase",str)

CapacitorBase::CapacitorBase( QString type, QString id )
             : Reactive( type, id )
{
    m_area = QRectF(-10,-8, 20, 16 );

    m_pin[0]->setLength( 12 );
    m_pin[1]->setLength( 12 );

    m_value = m_capacitance = 0.00001; // Farads

    addPropGroup( { tr("Main"), {
        new DoubProp<CapacitorBase>("Capacitance", tr("Capacitance"), "µF"
                                   , this, &CapacitorBase::value, &CapacitorBase::setValue ),

        new DoubProp<CapacitorBase>("Resistance" , tr("Resistance"), "µΩ"
                                   , this, &CapacitorBase::resist , &CapacitorBase::setResist ),

        new DoubProp<CapacitorBase>("InitVolt"   , tr("Initial Voltage"), "V"
                                   , this, &CapacitorBase::initVolt, &CapacitorBase::setInitVolt ),

        //new DoubProp<CapacitorBase>("ReaStep", tr("Reactive Step"), "ns"
        //                           , this, &CapacitorBase::reaStep , &CapacitorBase::setReaStep,0 )
    },0 } );

    setShowProp("Capacitance");
    setPropStr( "Capacitance", "10 µF" );
}
CapacitorBase::~CapacitorBase(){}

void CapacitorBase::setCurrentValue( double c )
{
    m_capacitance = c;
    m_changed = true;
}

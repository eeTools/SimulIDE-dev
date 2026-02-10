/***************************************************************************
 *   Copyright (C) 2017 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>

#include "adc.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "iopin.h"

#include "doubleprop.h"
#include "intprop.h"

#define tr(str) simulideTr("ADC",str)

Component* ADC::construct( QString type, QString id )
{ return new ADC( type, id ); }

LibraryItem* ADC::libraryItem()
{
    return new LibraryItem(
        tr("ADC"),
        "Other Logic",
        "1to3.png",
        "ADC",
        ADC::construct );
}

ADC::ADC( QString type, QString id )
   : LogicComponent( type, id )
{
    m_width  = 4;
    m_height = 9;

    m_maxVolt = 5;

    setLabelPos(-16,-80, 0);
    setNumOutputs( 8 );    // Create Output Pins
    setNumInps( 1, "In" );

    addPropGroup( { tr("Main"), {
        new IntProp <ADC>("Num_Bits", tr("Size"),"_bits"
                         , this, &ADC::numOuts, &ADC::setNumOutputs, propNoCopy,"uint" ),

        new DoubProp<ADC>("Vref", tr("Reference Voltage"),"V"
                         , this, &ADC::maxVolt, &ADC::setMaxVolt ),
    },groupNoCopy} );

    appendPropGroup( tr("Main"), IoComponent::familyProps() );

    addPropGroup( { tr("Outputs")
                  , IoComponent::outputProps()
                  + IoComponent::outputType(),0 } );

    addPropGroup( { tr("Timing"), IoComponent::edgeProps(),0 } );
}
ADC::~ADC(){}

void ADC::stamp()
{
    m_inpPin[0]->changeCallBack( this );
    LogicComponent::stamp();
}

void ADC::voltChanged()
{
    double volt = m_inpPin[0]->getVoltage();
    m_nextOutVal = volt*m_ratio+0.1;
    if( m_nextOutVal > m_maxValue ) m_nextOutVal = m_maxValue;
    LogicComponent::scheduleOutPuts( this );
}

void ADC::setNumOutputs( int outs )
{
    if( outs < 1 ) return;
    m_maxValue = pow( 2, outs )-1;
    m_ratio = m_maxValue/m_maxVolt;
    IoComponent::setNumOuts( outs, "D" );
    updtInPins();
}

void ADC::setMaxVolt( double v )
{
    m_maxVolt = v;
    m_ratio = m_maxValue/m_maxVolt;
}


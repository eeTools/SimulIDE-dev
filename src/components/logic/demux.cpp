/***************************************************************************
 *   Copyright (C) 2016 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <math.h>

#include "demux.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "circuit.h"
#include "connector.h"
#include "iopin.h"

#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("Demux",str)

Component* Demux::construct( QString type, QString id )
{ return new Demux( type, id ); }

LibraryItem* Demux::libraryItem()
{
    return new LibraryItem(
        tr("Demux"),
        "Converters",
        "demux.png",
        "Demux",
        Demux::construct );
}

Demux::Demux( QString type, QString id )
     : LogicComponent( type, id )
{
    m_width  = 4;
    m_height = 10;
    m_tristate = true;

    init({          // Inputs:
            "ID03S0",
            "ID02S1",
            "ID01S2",
            "IL05DI"
        });

    m_inpPin[1]->setLength( 6 );
    m_inpPin[0]->setLength( 4 );

    setNumOuts( 8 );
    createOePin("IU01OE ", id+"-in4");

    m_addrBits = 0;
    setAddrBits( 3 );

    addPropGroup( { tr("Main"), {
        new IntProp<Demux>("Address_Bits", tr("Address Size"),"_bits"
                          , this , &Demux::addrBits, &Demux::setAddrBits, propNoCopy,"uint" )
    }, groupNoCopy } );

    appendPropGroup( tr("Main"), IoComponent::familyProps() );

    addPropGroup( { tr("Inputs"),
        IoComponent::inputProps()
    ,0 } );

    addPropGroup( { tr("Outputs")
        , IoComponent::outputProps()
        + IoComponent::outputType()
        + QList<ComProperty*>({
            new BoolProp<Demux>("Tristate", tr("Tristate"),""
                                , this, &Demux::tristate, &Demux::setTristate ),
        })
    ,0 } );

    addPropGroup( { tr("Timing")  , IoComponent::edgeProps(),0 } );
}
Demux::~Demux(){}

void Demux::stamp()
{
    for( int i=0; i<4; ++i ) m_inpPin[i]->changeCallBack( this );
    LogicComponent::stamp();
}

void Demux::voltChanged()
{
    if( m_tristate ) LogicComponent::updateOutEnabled();

    int address = 0;

    if( m_inpPin[3]->getInpState() )
    {
        for( int i=0; i<3; ++i )
            if( m_inpPin[i]->getInpState() ) address += pow( 2, i );

        m_nextOutVal = 1<<address;
    }
    else m_nextOutVal = 0;

    scheduleOutPuts( this );
}

void Demux::setAddrBits( int bits )
{
    if( m_addrBits == bits ) return;

    if     ( bits < 1 ) bits = 1;
    else if( bits > 3 ) bits = 3;

    int channels = pow( 2, bits );
    m_addrBits = bits;

    m_height = channels+2;

    int w = m_width*8/2;
    int h = m_height*8/2;

    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    for( int i=0; i<3; ++i )
    {
        Pin* pin = m_inpPin[i];
        if( i < bits )
        {
            pin->setVisible( true );
            pin->setY( h+8 );
        }else{
            pin->removeConnector();
            pin->setVisible( false );
    }   }
    for( int i=0; i<8; ++i )
    {
        Pin* pin = m_outPin[i];
        if( i < channels )
        {
            pin->setVisible( true );
            pin->setY( i*8-(bits+bits/3)*8 );
        }else{
            pin->removeConnector();
            pin->setVisible( false );
    }   }
    m_oePin->setY( -h-8 ); // OE

    m_area = QRect(-w-1,-h-8-1, w*2+2, h*2+16+2 );
    Circuit::self()->update();
}

QPainterPath Demux::shape() const
{
    QPainterPath path;

    int w = m_width*8/2;
    int h = m_height*8/2;
    
    QVector<QPointF> points;
    points << QPointF(-w,-h+2 )
           << QPointF(-w, h-2 )
           << QPointF( w, h+6 )
           << QPointF( w,-h-6 );
        
    path.addPolygon( QPolygonF(points) );
    path.closeSubpath();
    return path;
}

void Demux::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Component::paint( p, option, widget );

    int w = m_width*8/2;
    int h = m_height*8/2;

    QPointF points[4] = {
        QPointF(-w,-h+2 ),
        QPointF(-w, h-2 ),
        QPointF( w, h+6 ),
        QPointF( w,-h-6 )};

    p->drawPolygon(points, 4);

    Component::paintSelected( p );
}

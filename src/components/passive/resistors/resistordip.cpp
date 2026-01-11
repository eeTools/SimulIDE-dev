/***************************************************************************
 *   Copyright (C) 2016 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "resistordip.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "circuit.h"
#include "pin.h"
#include "resistor.h"
#include "e-resistor.h"
#include "label.h"
#include "propdialog.h"
#include "mainwindow.h"

#include "doubleprop.h"
#include "intprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("ResistorDip",str)

eNode ResistorDip::m_puEnode("");

Component* ResistorDip::construct( QString type, QString id )
{ return new ResistorDip( type, id ); }

LibraryItem* ResistorDip::libraryItem()
{
    return new LibraryItem(
        tr("ResistorDip"),
        "Resistors",
        "resistordip.png",
        "ResistorDip",
        ResistorDip::construct);
}

ResistorDip::ResistorDip( QString type, QString id )
           : Component( type, id )
           , eElement( id )
{
    m_pullUp = false;
    m_puVolt = 5;

    m_size = 0;
    setSize( 8 );

    setLabelPos(-24,-40, 0);
    //setValLabelPos( 4,-26, 90 );
    //m_valLabel->setAcceptedMouseButtons( 0 );

    //QFont font = m_valLabel->font();
    //font.setPixelSize( 6 );
    //m_valLabel->setFont( font );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
        new DoubProp<ResistorDip>("Resistance", tr("Resistance"), "Ω"
                                 , this, &ResistorDip::resistance, &ResistorDip::setResistance ),

        new IntProp <ResistorDip>("Size", tr("Size"), ""
                                 , this, &ResistorDip::size, &ResistorDip::setSize, propNoCopy,"uint" ),

        new BoolProp<ResistorDip>("PullUp", tr("Pullup"), ""
                                 , this, &ResistorDip::pullUp, &ResistorDip::setPullUp ),

        new DoubProp<ResistorDip>("PuVolt", tr("Pullup Voltage"), "V"
                                 , this, &ResistorDip::puVolt, &ResistorDip::setPuVolt ),
    },0 } );

    setShowProp("Resistance");
    setPropStr( "Resistance", "100" );
}
ResistorDip::~ResistorDip(){}

void ResistorDip::stamp()
{
    if( !m_pullUp ) return;
    m_puEnode.setVolt( m_puVolt );
    for( int i=0; i<m_size; i++ )
    {
        int index = i*2;
        m_pin[index+1]->setEnode( &m_puEnode );
        m_pin[index]->createCurrent();
        m_pin[index]->stampCurrent( m_puVolt/m_resistance );
    }
}

void ResistorDip::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    for( eResistor* res : m_resistor ) res->setResistance( m_resistance );
    stamp();
}

void ResistorDip::createResistors( int c )
{
    int start = m_size;
    m_size = m_size+c;
    m_resistor.resize( m_size );
    m_pin.resize( m_size*2 );

    for( int i=start; i<m_size; i++ )
    {
        int index = i*2;
        QString reid = m_id;
        reid.append(QString("-resistor"+QString::number(i)));
        m_resistor[i] = new eResistor( reid );
        m_resistor[i]->setResistance( m_resistance );

        m_pin[index] = new Pin( 180, QPoint(-16,-32+8+i*8 ), reid+"-ePin"+QString::number(index), 0, this);
        m_resistor[i]->setEpin( 0, m_pin[index] );

        m_pin[index+1] = new Pin( 0, QPoint( 16,-32+8+i*8 ), reid+"-ePin"+QString::number(index+1), 0, this);
        m_pin[index+1]->setEnabled( !m_pullUp ); //
        m_pin[index+1]->setVisible( !m_pullUp ); //
        m_resistor[i]->setEpin( 1, m_pin[index+1] );
}   }

void ResistorDip::deleteResistors( int d )
{
    if( d > m_size ) d = m_size;
    int start = m_size-d;

    for( int i=start*2; i<m_size*2; ++i ) deletePin( m_pin[i] );
    for( int i=start;   i<m_size;   ++i ) delete m_resistor[i];
    m_size = m_size-d;
    m_resistor.resize( m_size );
    m_pin.resize( m_size*2 );
}

void ResistorDip::setResistance( double resist )
{
    if( resist < 1e-12 ) resist = 1e-12;
    if( m_resistance == resist ) return;
    m_resistance = resist;
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void ResistorDip::setSize( int size )
{
    if( size == m_size || size < 1 ) return;
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < m_size ) deleteResistors( m_size-size );
    else if( size > m_size ) createResistors( size-m_size );
    
    m_area = QRect( -10, -30, 20, m_size*8+4 );
    Circuit::self()->update();
}

void ResistorDip::setPullUp( bool p )
{
    if( m_pullUp == p ) return;
    m_pullUp = p;

    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    updtProperties();

    for( int i=0; i<m_size; i++ )
    {
        int index = i*2+1;
        m_pin[index]->setEnabled( !p );
        m_pin[index]->setVisible( !p );
        if( p ) m_pin[index]->removeConnector();
        else    m_pin[index]->setEnode( nullptr );
    }
    update();
}

void ResistorDip::updtProperties()
{
    if( !m_propDialog ) return;
    m_propDialog->showProp("PuVolt", m_pullUp );
    m_propDialog->adjustWidgets();
}

void ResistorDip::slotProperties()
{
    Component::slotProperties();
    updtProperties();
}

void ResistorDip::remove()
{
    deleteResistors( m_size );
    Component::remove();
}

void ResistorDip::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    if( m_ansiSymbol ){
        for( int i=0; i<m_size; i++ ) Resistor::drawAnsi( p, 0,-32+8+i*8, 0.7, 0.6 );
    }
    else p->drawRoundedRect( QRect(-9,-28, 18, m_size*8 ), 2, 2 );

    if( m_pullUp )
    {
        p->setRenderHint( QPainter::Antialiasing, true );
        QPen pen( Qt::black, 0.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p->setPen( pen );
        if( m_puVolt > 2.5 ){
            p->setBrush( QColor( 200, 50, 50 ) );
            p->drawRoundedRect( 3,-26, 4, 4, 2, 2 );
        }else{
            p->setBrush( QColor( 50,  50, 200 ) );
            p->drawRoundedRect(-7,-26, 4, 4, 2, 2 );
        }
    }

    Component::paintSelected( p );
}

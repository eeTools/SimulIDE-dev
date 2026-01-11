/***************************************************************************
 *   Copyright (C) 2022 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "connbase.h"
#include "circuitwidget.h"
#include "simulator.h"
#include "circuit.h"

#include "intprop.h"

#define tr(str) simulideTr("ConnBase",str)

ConnBase::ConnBase( QString type, QString id )
        : Component( type, id )
        , eElement( id )
{
    m_graphical = true;
    //this->setZValue(-1 );

    m_size = 0;
    setSize( 8 );
    setLabelPos(-16,-44, 0);

    addPropGroup( { tr("Main"), {
        new IntProp<ConnBase>("Size", tr("Size"),"_Pins"
                             , this, &ConnBase::size, &ConnBase::setSize, propNoCopy,"uint" )
    }, groupNoCopy } );
}
ConnBase::~ConnBase()
{
    m_connPins.clear();
}

void ConnBase::registerEnode( eNode* enode, int n )
{
    if( n < m_size )
    {
        if( m_sockPins[n]->conPin() ) m_sockPins[n]->registerPinsW( enode, n );
    }else{
        int nS = n- m_size;
        if( m_pin[nS]->conPin() ) m_pin[nS]->registerPinsW( enode, nS );
    }
}

void ConnBase::createPins( int c )
{
    int start = m_size;
    m_size = m_size+c;
    m_pin.resize( m_size*2 );
    m_sockPins.resize( m_size );

    for( int i=start; i<m_size; i++ )
    {
        m_pin[i] = new Pin( 180, QPoint(-8,-32+8+i*8 ), m_id+"-pin"+QString::number(i), i, this );
        m_sockPins[i] = new Pin( 90, QPoint( 0,-32+8+i*8 ), m_id+"-pin"+QString::number(i+m_size), i+m_size, this );
        m_sockPins[i]->setFlag( QGraphicsItem::ItemStacksBehindParent, false );
        m_sockPins[i]->setLength( 1 );
        m_sockPins[i]->setPinType( m_pinType );
    }
}

void ConnBase::deletePins( int d )
{
    if( d > m_size ) d = m_size;
    int start = m_size-d;

    for( int i=start; i<m_size; i++ )
    {
        deletePin( m_pin[i] );
        deletePin( m_sockPins[i] );
    }
    m_size = m_size-d;
    m_pin.resize( m_size*2 );
    m_sockPins.resize( m_size );
    
    Circuit::self()->update();
}

void ConnBase::setSize( int size )
{
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();
    
    if( size == 0 ) size = 8;
    
    if     ( size < m_size ) deletePins( m_size-size );
    else if( size > m_size ) createPins( size-m_size );

    for( int i=m_size; i<m_size*2; i++ )
    {
        m_pin[i] = m_sockPins[i-m_size];
        m_pin[i]->setIndex( i );
    }
    m_connPins.resize( size );
    m_area = QRectF(-4, -28, 8, m_size*8 );
    updatePins();

    Circuit::self()->update();
}

void ConnBase::setHidden( bool hid, bool hidArea, bool hidLabel )
{
    Component::setHidden( hid, hidArea, hidLabel );

    Component* parentComp = static_cast<Component*>( parentItem() );
    if( !parentComp ) return;

    for( int i=0; i<m_size; i++ )
    {
        m_sockPins[i]->setVisible( true );

        if( hid ) parentComp->addSignalPin( m_sockPins[i] );
        else      parentComp->remSignalPin( m_sockPins[i] );
    }
}

void ConnBase::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    //p->drawRoundRect( m_area, 4, 4 );

    updatePixmap();

    QPen pen( Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
    p->setPen( pen );
    if( !this->m_hidden ) p->drawRoundedRect( m_area, 4, 4 );

    for( int i=0; i<m_size; i++ )
    {
        p->drawPixmap( m_area.x(), m_area.y()+i*8, 8, 8, m_pinPixmap );
        //if( m_connPins[i] ) p->setBrush( QColor( 170, 170, 200 ) );
        //else                p->setBrush( Qt::black );
        //p->drawRoundRect(-2,-28+2+i*8, 3, 4, 1, 1 );
    }
    Component::paintSelected( p );
}

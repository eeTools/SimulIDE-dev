/***************************************************************************
 *   Copyright (C) 2018 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <QPushButton>
#include <QGraphicsProxyWidget>

#include "switchdip.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "circuit.h"
#include "e-node.h"
#include "pin.h"

#include "intprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("SwitchDip",str)

Component* SwitchDip::construct( QString type, QString id )
{ return new SwitchDip( type, id ); }

LibraryItem* SwitchDip::libraryItem()
{
    return new LibraryItem(
        tr("Switch Dip"),
        "Switches",
        "switchdip.png",
        "SwitchDip",
        SwitchDip::construct);
}

SwitchDip::SwitchDip( QString type, QString id )
         : Component( type, id )
         , eElement( id )
{
    m_graphical = true;
    m_changed   = true;

    m_commonPin = false;
    m_exclusive = false;

    m_color = QColor( 100, 100, 120 );
    m_size = 0;
    m_state = 0;
    setSize( 8 );
    setLabelPos(-16,-44, 0);
    
    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
        new IntProp <SwitchDip>("Size", tr("Size"), ""
                               , this, &SwitchDip::size, &SwitchDip::setSize, propNoCopy,"uint" ),

        new BoolProp<SwitchDip>("Exclusive", tr("Exclusive"), ""
                               , this, &SwitchDip::exclusive, &SwitchDip::setExclusive ),

        new BoolProp<SwitchDip>("CommonPin", tr("Common Pin"), ""
                               , this, &SwitchDip::commonPin, &SwitchDip::setCommonPin ),
    }, groupNoCopy } );

    addPropGroup( {"Hidden", {
new IntProp<SwitchDip>("State","","", this, &SwitchDip::state, &SwitchDip::setState,0,"uint" )
    }, groupHidden} );
}
SwitchDip::~SwitchDip(){}

void SwitchDip::stamp()
{
    eNode* node0 = m_pin[0]->getEnode();
    for( int i=0; i<m_size; i++ )
    {
        int pin1 = i*2;
        int pin2 = pin1+1;

        if( m_commonPin ) m_pin[pin1]->setEnode( node0 );
        else              node0 = m_pin[pin1]->getEnode();
        eNode* node1 = m_pin[pin2]->getEnode();

        //if( node0 ) node0->setSwitched( true );
        //if( node1 ) node1->setSwitched( true );

        m_pin[pin1]->setEnodeComp( node1 );
        m_pin[pin2]->setEnodeComp( node0 );
    }
    m_changed = true;
    updateStep();
}

void SwitchDip::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    int i = 0;
    for( QPushButton* button : m_buttons )
    {
        bool   state = m_state & 1<<i;
        double admit = 0;

        if( state  ){
            button->setIcon( QIcon(":/switchbut.png") );
            admit = 1e3;
        }else{
            button->setIcon( QIcon(":/stop.svg") );
        }
        button->setChecked( state );

        int pin = i*2;
        m_pin[pin]->stampAdmitance( admit );
        m_pin[pin+1]->stampAdmitance( admit );

        i++;
    }
}

void SwitchDip::onbuttonclicked()
{
    int i = 0;
    for( QPushButton* button : m_buttons )
    {
        bool state = button->isChecked();
        if( m_exclusive )
        {
            if( state == (bool)(m_state & 1<<i) )
                state = false;
        }

        if( state  )  m_state |= 1<<i;
        else          m_state &= ~(1<<i);

        i++;
    }
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void SwitchDip::setState( int state )
{
    if( m_state == state ) return;
    m_state = state;

    for( QPushButton* button : m_buttons )
    {
        bool switchState = state&1;
        state >>= 1;
        
        button->setChecked( switchState );
        
        if( switchState  ) button->setIcon(QIcon(":/switchbut.png"));
        else               button->setIcon(QIcon(":/stop.svg"));
    }
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void SwitchDip::createSwitches( int c )
{
    int start = m_size;
    m_size = m_size+c;
    m_pin.resize( m_size*2 );

    for( int i=start; i<m_size; i++ )
    {
        int index = i*2;
        QString butId = m_id + QString("-switch"+QString::number(i));
        QPushButton* button = new QPushButton( );
        button->setMaximumSize( 6, 6 );
        button->setGeometry(-6,-6, 6, 6);

        button->setCursor( Qt::PointingHandCursor );
        button->setCheckable( true );
        button->setChecked( true );
        button->setIcon(QIcon(":/switchbut.png"));
        m_buttons.append( button );
        
        QGraphicsProxyWidget* proxy = Circuit::self()->addWidget( button );
        proxy->setParentItem( this );
        proxy->setPos( QPoint( 3,-27+i*8 ) );
        m_proxys.append( proxy );
        QObject::connect( button, &QPushButton::released, [=](){ onbuttonclicked(); });

        Pin* pin = new Pin( 180, QPoint(-8,-32+8+i*8 ), butId+"-pinP", 0, this, 5 );
        pin->setVisible( !m_commonPin );
        m_pin[index] = pin;

        pin = new Pin( 0, QPoint( 16,-32+8+i*8 ), butId+"-pinN", 0, this, 5 );
        m_pin[index+1] = pin;
        
        m_state |= 1<<i;                          // default state = on;
}   }

void SwitchDip::deleteSwitches( int d )
{
    if( d > m_size ) d = m_size;
    int start = m_size-d;

    for( int i=start*2; i<m_size*2; i++ ) deletePin( m_pin[i] );

    for( int i=start; i<m_size; i++ )
    {
        delete m_buttons.takeLast();
        m_proxys.removeLast();
    }
    m_size = m_size-d;
    m_pin.resize( m_size*2 );
    
    Circuit::self()->update();
}

void SwitchDip::setSize( int size )
{
    if( size < 1 ) size = 1;
    if( m_size == size ) return;

    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if     ( size < m_size ) deleteSwitches( m_size-size );
    else if( size > m_size ) createSwitches( size-m_size );
    
    m_area = QRectF(-3,-28, 14, m_size*8 );

    Circuit::self()->update();
}

void SwitchDip::setExclusive( bool e )
{
    if( m_exclusive == e ) return;
    m_exclusive = e;
    if( e ) m_state = 0;
    m_changed = true;
    if( !Simulator::self()->isRunning() ) updateStep();
}

void SwitchDip::setCommonPin( bool c )
{
    if( m_commonPin == c ) return;
    m_commonPin = c;

    for( int i=2; i<m_size*2; i+=2 )
    {
        if( c ) m_pin[i]->removeConnector();
        m_pin[i]->setVisible( !c );
    }
}

void SwitchDip::setHidden( bool hid, bool hidArea, bool hidLabel )
{
    Component::setHidden( hid, hidArea, hidLabel );
    { for( int i=2; i<m_size*2; i+=2 )  m_pin[i]->setVisible( !m_commonPin && !hid ); }
}

void SwitchDip::setLinkedValue( double v, int i )
{
    if( i == 0 ) { int p = v; m_state = 1<<p; }
    else         m_state = v;
    m_changed = true;
}

void SwitchDip::remove()
{
    deleteSwitches( m_size );
    Component::remove();
}

void SwitchDip::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    p->drawRoundedRect( m_area, 4, 4 );

    Component::paintSelected( p );
}

/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "clock-base.h"
#include "iopin.h"
#include "simulator.h"
#include "custombutton.h"

#include "boolprop.h"

ClockBase::ClockBase( QString type, QString id )
         : FixedVolt( type, id )
{
    m_area = QRect(-14,-8, 22, 16 );

    m_graphical = true;
    m_isRunning = false;
    m_alwaysOn  = false;

    m_psPerCycleInt = 0;
    ClockBase::setFreq( 1000 );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { "Hidden1", {
        new BoolProp<ClockBase>("Running", "",""
                               , this, &ClockBase::running, &ClockBase::setRunning ),
    }, groupHidden} );
}
ClockBase::~ClockBase(){}

bool ClockBase::propNotFound( QString prop, QString val )
{
    if( prop =="Out" )
    {
        setRunning( val == "true" );   // Old: TODELETE
        return true;
    }
    return FixedVolt::propNotFound( prop, val );
}

void ClockBase::stamp()
{
    setFreq( m_freq );
    if( !Simulator::self()->isPaused() ) m_changed = true;
}

void ClockBase::setAlwaysOn( bool on )
{
    m_alwaysOn = on;
    if( on ) setRunning( on );
    m_button->setVisible( !on );
}

void ClockBase::setFreq( double freq )
{
    m_psPerCycleDbl = 1e6*1e6/freq;
    m_psPerCycleInt = m_psPerCycleDbl;
    
    m_freq = freq;
    m_remainder = 0;

    setRunning( m_isRunning || m_alwaysOn );
}

void ClockBase::setRunning( bool running )
{
    running = running && (m_freq>0);
    m_button->setChecked( running );
    m_isRunning = running;
    m_changed = true;
    update();
}

void ClockBase::setLinkedValue( double v, int )
{
    setFreq( v );
}

void ClockBase::setHidden( bool hide, bool hidArea, bool hidLabel )
{
    Component::setHidden( hide, hidArea, hidLabel );
    if  ( hidArea ) m_area = QRectF( 0, 0,-4,-4 );     // Totally hidden
    else if( hide ) m_area = QRectF(-30,-6, 12, 12 );  // In Board
    else            m_area = QRect(-14,-8, 22, 16 );   // Normal

    m_proxy->setFlag( QGraphicsItem::ItemStacksBehindParent, hide && !hidArea );
}

void ClockBase::onbuttonclicked() { setRunning( !m_isRunning ); }
